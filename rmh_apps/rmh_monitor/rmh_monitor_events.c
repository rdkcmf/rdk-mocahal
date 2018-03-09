/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#include <pthread.h>
#include <sys/time.h>
#include "rmh_monitor.h"

/**
 * The frequency in minutes that full network status should be dumpped in a stable environment.
 */
#define RMH_MONITOR_STATUS_DUMP_MIN 60

/**
 * The frequency in minutes that a one line 'ping' message will appear in the logs when everything is stable.
 */
#define RMH_MONITOR_STATUS_PING_MIN 2


/**
 * The amount of time with no messages printed to the log that we will consider the network 'stable'
 */
#define RMH_MONITOR_MIN_NETWORK_STABALIZE_SEC 10


/*******************************************************************************************************************
*
* Event Functions
*
*    These fuctions are used to track events and control how they are managed.
*******************************************************************************************************************/

/**
 * This function is called to update the self node value
*/
static inline
bool RMHMonitor_Event_SelfNodeId(RMHMonitor *app, struct timeval *time, const uint32_t nodeId) {
    char printBuff[32];
    RMH_Result ret;

    /* Ensure the link is up soemthing's actually changed */
    if ((app->linkStatus != RMH_LINK_STATUS_UP) ||
        (app->netStatus.selfNodeId == nodeId && app->netStatus.nodes[app->netStatus.selfNodeId].joined)) {
        return false;
    }

    /* Get some information about the node */
    ret=RMH_Interface_GetMac(app->rmh, &app->netStatus.nodes[nodeId].mac);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Interface_GetMac failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_Self_GetPreferredNCEnabled(app->rmh, &app->netStatus.nodes[nodeId].preferredNC);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Self_GetPreferredNCEnabled failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_Self_GetHighestSupportedMoCAVersion(app->rmh, &app->netStatus.nodes[nodeId].mocaVersion);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Self_GetHighestSupportedMoCAVersion failed -- %s!\n", RMH_ResultToString(ret));
    }

    RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s **Self Node\n", nodeId, RMH_MoCAVersionToString(app->netStatus.nodes[nodeId].mocaVersion),
                                                        RMH_MacToString(app->netStatus.nodes[nodeId].mac, printBuff, sizeof(printBuff)),
                                                        app->netStatus.nodes[nodeId].preferredNC ? "TRUE" : "FALSE");

    app->netStatus.selfNodeId=nodeId;
    app->netStatus.nodes[nodeId].joined=true;
    return true;
}


/**
 * This function is called when we expect that a node has joined the network
*/
static inline
bool RMHMonitor_Event_JoinNode(RMHMonitor *app, struct timeval *time, const uint32_t nodeId) {
    char printBuff[32];
    RMH_Result ret;

    /* If the link is down do nothing */
    if (app->linkStatus != RMH_LINK_STATUS_UP) {
        return false;
    }

    if (app->netStatus.nodes[nodeId].joined) {
        /*RMH_PrintWrn("Got 'join' notification for node ID %u which was already on the network\n", nodeId);*/
        return false;
    }

    /* Get some information about the node */
    ret=RMH_RemoteNode_GetMac(app->rmh, nodeId, &app->netStatus.nodes[nodeId].mac);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_RemoteNode_GetMac failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_RemoteNode_GetPreferredNC(app->rmh, nodeId, &app->netStatus.nodes[nodeId].preferredNC);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_RemoteNode_GetPreferredNC failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_RemoteNode_GetHighestSupportedMoCAVersion(app->rmh, nodeId, &app->netStatus.nodes[nodeId].mocaVersion);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_RemoteNode_GetHighestSupportedMoCAVersion failed -- %s!\n", RMH_ResultToString(ret));
    }

    RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s\n", nodeId, RMH_MoCAVersionToString(app->netStatus.nodes[nodeId].mocaVersion),
                                                        RMH_MacToString(app->netStatus.nodes[nodeId].mac, printBuff, sizeof(printBuff)),
                                                        app->netStatus.nodes[nodeId].preferredNC ? "TRUE" : "FALSE");

    app->netStatus.nodes[nodeId].joined=true;
    return true;
}


/**
 * This function is called when we expect that a node has dropped from the network
*/
static inline
bool RMHMonitor_Event_DropNode(RMHMonitor *app, struct timeval *time, const uint32_t nodeId) {
    char printBuff[32];

    /* If the link is down do nothing */
    if (app->linkStatus != RMH_LINK_STATUS_UP) {
        return false;
    }

    if (app->netStatus.nodes[nodeId].joined) {
        RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s %s\n", nodeId, RMH_MoCAVersionToString(app->netStatus.nodes[nodeId].mocaVersion),
                                                            RMH_MacToString(app->netStatus.nodes[nodeId].mac, printBuff, sizeof(printBuff)),
                                                            app->netStatus.nodes[nodeId].preferredNC ? "TRUE" : "FALSE",
                                                            app->netStatus.selfNodeId == nodeId ? "**Self node" : "");
    }
    else {
        RMH_PrintWrn("Got 'drop' notification for node ID %u which is not on the network\n", nodeId);
    }

    app->netStatus.nodes[nodeId].joined=false;
    return true;
}


/**
 * This function is called when we expect that MoCA 'network NC' has changed
*/
static inline
bool RMHMonitor_Event_NCChanged(RMHMonitor *app, struct timeval *time, const uint32_t ncNodeId) {
    char oldNC[32];
    char newNC[32];

    /* Ensure the link is up soemthing's actually changed */
    if ((app->linkStatus != RMH_LINK_STATUS_UP) ||
        (app->netStatus.ncNodeIdValid && app->netStatus.ncNodeId == ncNodeId)) {
        return false;
    }

    if (app->netStatus.ncNodeIdValid) {
        RMH_PrintMsgT(time, "NC is now node:%02u %s [Previous:%02u %s]\n", ncNodeId, RMH_MacToString(app->netStatus.nodes[ncNodeId].mac, newNC, sizeof(newNC)),
                                                                    app->netStatus.ncNodeId, RMH_MacToString(app->netStatus.nodes[app->netStatus.ncNodeId].mac, oldNC, sizeof(oldNC)));
    }
    else {
        RMH_PrintMsgT(time, "NC is now node:%02u %s\n", ncNodeId, RMH_MacToString(app->netStatus.nodes[ncNodeId].mac, newNC, sizeof(newNC)));
    }
    app->netStatus.ncNodeId=ncNodeId;
    app->netStatus.ncNodeIdValid=true;
    return true;
}


/**
 * This function is called when we expect that MoCA 'network version' has changed
*/
static inline
bool RMHMonitor_Event_NetworkMoCAVersionChanged(RMHMonitor *app, struct timeval *time, const RMH_MoCAVersion mocaVersion) {
    /* Ensure the link is up soemthing's actually changed */
    if ((app->linkStatus != RMH_LINK_STATUS_UP) ||
        (app->netStatus.networkMoCAVerValid && app->netStatus.networkMoCAVer == mocaVersion)) {
        return false;
    }

    if (app->netStatus.networkMoCAVerValid) {
        RMH_PrintMsgT(time, "MoCA network is now %s [Previous: %s]\n", RMH_MoCAVersionToString(mocaVersion),
                                                                            RMH_MoCAVersionToString(app->netStatus.networkMoCAVer));
    }
    else {
        RMH_PrintMsgT(time, "MoCA network is now %s\n", RMH_MoCAVersionToString(mocaVersion));
    }

    app->netStatus.networkMoCAVer=mocaVersion;
    app->netStatus.networkMoCAVerValid=true;
    return true;
}


/**
 * This function is called when we expect that MoCA 'link' status has changed
*/
static inline
bool RMHMonitor_Event_LinkStatusChanged(RMHMonitor *app, struct timeval *time, const RMH_LinkStatus linkStatus) {
    /* Ensure soemthing's actually changed */
    if (app->linkStatusValid && app->linkStatus == linkStatus) {
        return false;
    }

    RMH_PrintMsgT(time, "MoCA link is now %s!\n", RMH_LinkStatusToString(linkStatus));
    app->linkStatus=linkStatus;
    app->linkStatusValid=true;

    if (app->linkStatus == RMH_LINK_STATUS_UP) {
        /* Link is up, update netstatus */
        RMH_Result ret;
        int i;
        uint32_t nodeId;
        RMH_MoCAVersion networkMoCAVer;
        RMH_NodeList_Uint32_t nodeIds;

        ret=RMH_Network_GetNodeId(app->rmh, &nodeId);
        if (ret != RMH_SUCCESS) {
            RMH_PrintErr("RMH_Interface_GetMac failed -- %s!\n", RMH_ResultToString(ret));
        }
        else {
            RMHMonitor_Event_SelfNodeId(app, time, nodeId);
        }

        ret=RMH_Network_GetRemoteNodeIds(app->rmh, &nodeIds);
        if (ret != RMH_SUCCESS) {
            RMH_PrintErr("RMH_Network_GetRemoteNodeIds failed -- %s!\n", RMH_ResultToString(ret));
        }
        else {
            for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                if (nodeIds.nodePresent[i]) {
                    RMHMonitor_Event_JoinNode(app, time, i);
                }
            }
        }

        ret=RMH_Network_GetNCNodeId(app->rmh, &nodeId);
        if (ret != RMH_SUCCESS) {
            RMH_PrintErr("RMH_Network_GetRemoteNodeIds failed -- %s!\n", RMH_ResultToString(ret));
        }
        else {
            RMHMonitor_Event_NCChanged(app, time, nodeId);
        }

        ret=RMH_Network_GetMoCAVersion(app->rmh, &networkMoCAVer);
        if (ret != RMH_SUCCESS) {
            RMH_PrintErr("RMH_Network_GetMoCAVersion failed -- %s!\n", RMH_ResultToString(ret));
        }
        else {
            RMHMonitor_Event_NetworkMoCAVersionChanged(app, time, networkMoCAVer);
        }
    }
    else {
        /* MoCA is down, invalidate net status */
        memset(&app->netStatus, 0, sizeof(app->netStatus));
    }
    return true;
}


/**
 * This function should print a single line status. Mainly to indicate the logger is still alive
 */
static
void RMHMonitor_Event_PrintPing(RMHMonitor *app, struct timeval *time) {
    bool mocaEnabled;
    if (RMH_Self_GetEnabled(app->rmh, &mocaEnabled) != RMH_SUCCESS || !mocaEnabled) {
        RMH_PrintMsgT(time, "Link:DISABLED\n");
    }
    else if (app->linkStatus != RMH_LINK_STATUS_UP) {
        RMH_PrintMsgT(time, "Link:DOWN\n");
    }
    else {
        uint32_t numNodes, ncNode, selfNode, uptime;
        int32_t numNodesPrint, ncNodePrint, selfNodePrint, uptimePrint;
        numNodesPrint = RMH_Network_GetNumNodes(app->rmh, &numNodes) == RMH_SUCCESS ? numNodes : -1;
        selfNodePrint = RMH_Network_GetNodeId(app->rmh, &selfNode)   == RMH_SUCCESS ? selfNode : -1;
        ncNodePrint =   RMH_Network_GetNCNodeId(app->rmh, &ncNode)   == RMH_SUCCESS ? ncNode   : -1;
        uptimePrint=    RMH_Network_GetLinkUptime(app->rmh, &uptime) == RMH_SUCCESS ? uptime   : 0;
        RMH_PrintMsgT(time, "Link:UP [%02uh:%02um:%02us] Nodes:%d Self:%d NC:%d\n", uptimePrint/3600, (uptimePrint%3600)/60, uptimePrint%60, numNodesPrint, selfNodePrint, ncNodePrint);
    }
}


/**
 * This function should print a full status summary.
 */
static
void RMHMonitor_Event_PrintFull(RMHMonitor *app, struct timeval *time) {
    bool mocaEnabled = false;
    RMH_Handle rmh=app->rmh;

    /*********************************************************************
     * We're seeing an issue where after some time (30+ minutes) the MoCA
     * driver is having issues using handles that have sat inactive. For
     * Now we'll just create a new temporary handle to dump the status.
     * Eventually, when this bug is fixed, we should remove this and use
     * the default handle 'app->rmh'
     **********************************************************************/
    rmh=RMH_Initialize(RMHMonitor_RMHCallback, app);
    if (rmh && RMH_Log_SetAPILevel(rmh, RMH_LOG_DEFAULT ) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set the log level on temporary status handle!\n");
        RMH_Destroy(rmh);
        rmh=NULL;
    }
    if (rmh && RMH_SetEventCallbacks(rmh, RMH_EVENT_API_PRINT) != RMH_SUCCESS) {
        RMH_PrintErr("Failed setting callback events on temporary status handle!\n");
        RMH_Destroy(rmh);
        rmh=NULL;
    }
    if (!rmh) {
        rmh=app->rmh;
    }
    /**********************************************************************/

    if (RMH_Self_GetEnabled(rmh, &mocaEnabled) == RMH_SUCCESS && mocaEnabled) {
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintStatus(rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintStats(rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintModulation(rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintFlows(rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
    }

    /**********************************************************************/
    if (rmh != app->rmh) {
        RMH_Destroy(rmh);
    }
    /**********************************************************************/
}


/**
 * This is the main event thread. It will monitor the event queue and when an even occurs it will take some action
*/
void RMHMonitor_Event_Thread(void * context) {
    RMHMonitor *app=(RMHMonitor *)context;
    RMHMonitor_CallbackEvent *cbE;
    RMH_Result ret;
    bool networkStable=false;
    bool mocaEnabled;
    char printBuff[128];
    bool printStatus = false;
    struct timeval now;
    struct timeval lastStatusPrint;
    struct timeval lastStatusPing;
    int threadRet=1;

    /* Reset internal status */
    app->linkStatusValid=false;
    memset(&app->netStatus, 0, sizeof(app->netStatus));

    /* Validate the handle */
    ret = RMH_ValidateHandle(app->rmh);
    if (ret == RMH_UNIMPLEMENTED || ret == RMH_NOT_SUPPORTED) {
        RMH_PrintWrn("RMH_ValidateHandle returned %s. We will not be able to monitor to make sure the handle remains valid\n", RMH_ResultToString(ret));
    }
    else if (ret != RMH_SUCCESS) {
        RMH_PrintErr("The RMH handle %p seems to no longer be valid. Aborting\n", app->rmh);
        goto exit_err;
    }

    /* Dump a full status to get things started */
    gettimeofday(&now, NULL);

    if (RMH_Self_GetEnabled(app->rmh, &mocaEnabled) != RMH_SUCCESS) {
        goto exit_err;
    }
    else if (mocaEnabled) {
        RMH_LinkStatus linkStatus;
        if (RMH_Self_GetLinkStatus(app->rmh, &linkStatus) == RMH_SUCCESS) {
            RMHMonitor_Event_LinkStatusChanged(app, &now, linkStatus);
        }
    }
    else {
        RMHMonitor_Event_LinkStatusChanged(app, &now, RMH_LINK_STATUS_DISABLED);
    }
    RMHMonitor_Event_PrintFull(app, &now);

    /* Looks like we're connected with a valid handle, reset the timer */
    app->reconnectSeconds=0;

    app->appPrefix=NULL;
    lastStatusPrint=now;
    lastStatusPing=now;

    /* Loop forever here until the thread is no longer running */
    while (app->eventThreadRunning) {
        ret=RMHMonitor_Semaphore_WaitTimeout(app->eventNotify, 2000);
        if (ret == RMH_FAILURE) {
            RMH_PrintErr("Failed in RMHMonitor_Semaphore_WaitTimeout\n");
        }

        gettimeofday(&now, NULL);

        ret = RMH_ValidateHandle(app->rmh);
        if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_NOT_SUPPORTED) {
            RMH_PrintErr("The RMH handle %p seems to no longer be valid. Aborting\n", app->rmh);
            goto exit_err;
        }

        /* We determine that the network is 'stable' when there hasn't been anything print to the
         * log in at least RMH_MONITOR_MIN_NETWORK_STABALIZE seconds. We can't just use the semaphore
         * timeout because we might get repeated messages from the MoCA driver which aren't being print
         * to the log (no change). In that case it will look like the log is frozen.
         */
        networkStable=(now.tv_sec-app->lastPrint.tv_sec) > RMH_MONITOR_MIN_NETWORK_STABALIZE_SEC;

        /* We want to first check if we have any pending prints. This will happen because either
         *    1) An event occurred which indicated a dump should happen
         *    2) It has been RMH_MONITOR_STATUS_DUMP_MIN minutes has passed since the last status dump
         *
         *   Note: Before we print anything make sure the network is stable. This is to make sure we don't
         *         flood logs with status updates if the network is changing frequently.
         */
        if ((now.tv_sec-lastStatusPrint.tv_sec) > RMH_MONITOR_STATUS_DUMP_MIN*60 ||
             (printStatus && networkStable)) {
            /* Dump the full status and update the last print time */
            RMHMonitor_Event_PrintFull(app, &now);
            app->appPrefix=NULL;
            lastStatusPrint=now;
            lastStatusPing=now;
            printStatus=false;
        }
        else if (networkStable && (lastStatusPing.tv_sec == 0 || (now.tv_sec-lastStatusPing.tv_sec) > RMH_MONITOR_STATUS_PING_MIN*60)) {
            /* Dump the ping status and update the last print time */
            RMHMonitor_Event_PrintPing(app, &now);
            lastStatusPing=now;
        }

        /* We're all caught up on prints, check if there are any events in the queue */
        cbE = app->eventQueue.tqh_first;
        if (cbE) {
            /* There is a pending event, handle it */
            switch(cbE->event) {
            case RMH_EVENT_ADMISSION_STATUS_CHANGED:
                RMH_PrintMsgT(&cbE->eventTime, "[ADMNSTUS] Admission status: %s\n", RMH_AdmissionStatusToString(cbE->eventData.RMH_EVENT_ADMISSION_STATUS_CHANGED.status));
                break;
            case RMH_EVENT_LINK_STATUS_CHANGED:
                app->appPrefix="[CHANGE] ";
                printStatus|=RMHMonitor_Event_LinkStatusChanged(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_LINK_STATUS_CHANGED.status);
                break;
            case RMH_EVENT_MOCA_RESET:
                RMH_PrintMsgT(&cbE->eventTime, "[*RESET* ] MoCA Reset triggered - %s\n", RMH_MoCAResetReasonToString(cbE->eventData.RMH_EVENT_MOCA_RESET.reason));
                break;
            case RMH_EVENT_MOCA_VERSION_CHANGED:
                app->appPrefix="[CHANGE] ";
                printStatus|=RMHMonitor_Event_NetworkMoCAVersionChanged(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_MOCA_VERSION_CHANGED.version);
                break;
            case RMH_EVENT_NODE_JOINED:
                app->appPrefix="[CHANGE] ";
                printStatus|=RMHMonitor_Event_JoinNode(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_NODE_JOINED.nodeId);
                break;
            case RMH_EVENT_NODE_DROPPED:
                app->appPrefix="[CHANGE] ";
                printStatus|=RMHMonitor_Event_DropNode(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_NODE_JOINED.nodeId);
                break;
            case RMH_EVENT_NC_ID_CHANGED:
                if (cbE->eventData.RMH_EVENT_NC_ID_CHANGED.ncValid) {
                    app->appPrefix="[CHANGE] ";
                    printStatus|=RMHMonitor_Event_NCChanged(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_NC_ID_CHANGED.ncNodeId);
                }
                break;
            case RMH_EVENT_LOW_BANDWIDTH:
                RMH_PrintMsgT(&cbE->eventTime, "WARNING: Low bandwidth reported\n");
                break;
            default:
                RMH_PrintMsgT(&cbE->eventTime, "WARNING: Unhandled MoCA event %s!\n", RMH_EventToString(cbE->event, printBuff, sizeof(printBuff)));
                break;
            }
            /* If we have a request to print the full status, keep the prefix and we'll clear it later. If not clear it now. */
            if (!printStatus) app->appPrefix=NULL;

            /* Clear this handled event off the queue */
            RMHMonitor_Queue_Dequeue(app);
        }
    }

    ret=0;
    pthread_exit(&threadRet);

exit_err:
    ret=1;
    pthread_exit(&threadRet);
}
