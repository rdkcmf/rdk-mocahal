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



/*******************************************************************************************************************
*
* Event Functions
*
*    These fuctions are used to track events and control how they are managed.
*******************************************************************************************************************/

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
        RMH_PrintWrn("Got 'join' notification for node ID %u which was already on the network\n", nodeId);
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
        RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s\n", nodeId, RMH_MoCAVersionToString(app->netStatus.nodes[nodeId].mocaVersion),
                                                            RMH_MacToString(app->netStatus.nodes[nodeId].mac, printBuff, sizeof(printBuff)),
                                                            app->netStatus.nodes[nodeId].preferredNC ? "TRUE" : "FALSE");
    }
    else if (app->netStatus.selfNodeId == nodeId) {
        RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s **Self node\n", nodeId, RMH_MoCAVersionToString(app->netStatus.self.mocaVersion),
                                                                               RMH_MacToString(app->netStatus.self.mac, printBuff, sizeof(printBuff)),
                                                                               app->netStatus.self.preferredNC ? "TRUE" : "FALSE");
    }
    else {
        RMH_PrintWrn("Got 'drop' notification for node ID %u which is not on the network\n", nodeId);
    }

    app->netStatus.nodes[nodeId].joined=false;
    return true;
}


/**
 * This function is called to update the self node value
*/
static inline
bool RMHMonitor_Event_SelfNodeId(RMHMonitor *app, struct timeval *time, const uint32_t selfNodeId) {
    char printBuff[32];
    RMH_Result ret;

    /* Ensure the link is up soemthing's actually changed */
    if ((app->linkStatus != RMH_LINK_STATUS_UP) ||
        (app->netStatus.selfNodeIdValid && app->netStatus.selfNodeId == selfNodeId)) {
        return false;
    }

    app->netStatus.selfNodeId=selfNodeId;
    app->netStatus.selfNodeIdValid =true;

    /* Get some information about the node */
    ret=RMH_Interface_GetMac(app->rmh, &app->netStatus.self.mac);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Interface_GetMac failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_Self_GetPreferredNCEnabled(app->rmh, &app->netStatus.self.preferredNC);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Self_GetPreferredNCEnabled failed -- %s!\n", RMH_ResultToString(ret));
    }

    ret=RMH_Self_GetHighestSupportedMoCAVersion(app->rmh, &app->netStatus.self.mocaVersion);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("RMH_Self_GetHighestSupportedMoCAVersion failed -- %s!\n", RMH_ResultToString(ret));
    }

    RMH_PrintMsgT(time, "Node:%02u %s MoCA:%s PNC:%s **Self Node\n", selfNodeId, RMH_MoCAVersionToString(app->netStatus.self.mocaVersion),
                                                                            RMH_MacToString(app->netStatus.self.mac, printBuff, sizeof(printBuff)),
                                                                            app->netStatus.self.preferredNC ? "TRUE" : "FALSE");
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
 * This function is called when we expect that MoCA 'enabled' status has changed
*/
static inline
bool RMHMonitor_Update_ReportMoCAEnabled(RMHMonitor *app, struct timeval *time, const bool mocaEnabled) {
    /* Ensure soemthing's actually changed */
    if (app->mocaEnabledValid && app->mocaEnabled == mocaEnabled) {
        return false;
    }

    /* Update our internal moca enabled state and check link status */
    app->mocaEnabled = mocaEnabled;
    app->mocaEnabledValid=true;
    if (app->mocaEnabled) {
        RMH_LinkStatus linkStatus;
        RMH_Result ret;

        ret=RMH_Self_GetLinkStatus(app->rmh, &linkStatus);
        if (ret == RMH_SUCCESS) {
            RMHMonitor_Event_LinkStatusChanged(app, time, linkStatus);
        }
        else {
            RMH_PrintErr("RMH_Self_GetLinkStatus failed -- %s!\n", RMH_ResultToString(ret));
        }
    }
    else {
        RMH_PrintMsgT(time, "** MoCA is disabled **\n");
        RMHMonitor_Event_LinkStatusChanged(app, time, RMH_LINK_STATUS_DISABLED);
    }

    return true;
}


/**
 * This function should print a single line status. Mainly to indicate the logger is still alive
 */
static
void RMHMonitor_Event_PrintPing(RMHMonitor *app, struct timeval *time) {
    if (!app->mocaEnabled) {
        RMH_PrintMsgT(time, "** MoCA is disabled **\n");
    }
    else if (app->linkStatus != RMH_LINK_STATUS_UP) {
        RMH_PrintMsgT(time, "** MoCA link is down **\n");
    }
    else {
        uint32_t numNodes;
        uint32_t ncNode;
        uint32_t selfNode;
        RMH_Network_GetNumNodes(app->rmh, &numNodes);
        RMH_Network_GetNodeId(app->rmh, &selfNode);
        RMH_Network_GetNCNodeId(app->rmh, &ncNode);
        RMH_PrintMsgT(time, "Link:UP Nodes:%u Self:%u NC:%u\n", numNodes, selfNode, ncNode);
    }
}


/**
 * This function should print a full status summary.
 */
static
void RMHMonitor_Event_PrintFull(RMHMonitor *app, struct timeval *time) {
    if (!app->mocaEnabled) {
        RMH_PrintMsgT(time, "** MoCA is disabled **\n");
    }
    else {
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintStatus(app->rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintStats(app->rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintModulation(app->rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
        RMH_Log_PrintFlows(app->rmh, NULL);
        RMH_PrintMsg("==============================================================================================================================\n");
    }
}


/**
 * This is the main event thread. It will monitor the event queue and when an even occurs it will take some action
*/
void RMHMonitor_Event_Thread(void * context) {
    RMHMonitor *app=(RMHMonitor *)context;
    RMHMonitor_CallbackEvent *cbE;
    RMH_Result ret;
    bool mocaEnabled;
    char printBuff[128];
    bool printStatus = false;
    struct timeval now;
    struct timeval lastStatusPrint;
    struct timeval lastStatusPing;

    /* Set the prefix to indicate we're currently in startup */
    app->appPrefix="[  INIT  ] ";

    /* Update MoCA enabled */
    if (RMH_Self_GetEnabled(app->rmh, &mocaEnabled) == RMH_SUCCESS) {
        RMHMonitor_Update_ReportMoCAEnabled(app, &now, mocaEnabled);
    }

    /* Dump a full status to get things started */
    gettimeofday(&now, NULL);
    RMHMonitor_Event_PrintFull(app, &now);
    app->appPrefix=NULL;
    lastStatusPrint=now;
    lastStatusPing=now;

    /* Loop forever here until the thread is no longer running */
    while (app->eventThreadRunning) {

        /* Wait for a MoCA event notification.Otherwise timeout every 5 seconds and check if there's any other pending work to do. */
        ret=RMHMonitor_Semaphore_WaitTimeout(app->eventNotify, 5000);
        if (ret == RMH_FAILURE) {
            RMH_PrintErr("Failed in RMHMonitor_Semaphore_WaitTimeout\n");
        }

        gettimeofday(&now, NULL);

        /* We want to first check if we have any pending prints. This will happen because either
         *    1) An event occurred which indicated a dump should happen
         *    2) It has been RMH_MONITOR_STATUS_DUMP_MIN minutes has passed since the last status dump
         *
         *   Note: When an event occurs and requests a status dump, it doesn't happen immediately. We will wait for
         *         a timeout event on the semaphore. This ensure that the network has had some amount of stabillity
         *         for a few seconds and we won't get flooded with large status dumps.
         *
         *   If we aren't printing a full status we will print a ping message every RMH_MONITOR_STATUS_PING_MIN minutes
         */
        if ((now.tv_sec-lastStatusPrint.tv_sec) > RMH_MONITOR_STATUS_DUMP_MIN*60 ||
             (printStatus && ret==RMH_TIMEOUT)) {
            /* Dump the full status and update the last print time */
            RMHMonitor_Event_PrintFull(app, &now);
            app->appPrefix=NULL;
            lastStatusPrint=now;
            lastStatusPing=now;
            printStatus=false;
        }
        else if (ret==RMH_TIMEOUT && (lastStatusPing.tv_sec == 0 || (now.tv_sec-lastStatusPing.tv_sec) > RMH_MONITOR_STATUS_PING_MIN*60)) {
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
                app->appPrefix="[**LINK**] ";
                printStatus|=RMHMonitor_Event_LinkStatusChanged(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_LINK_STATUS_CHANGED.status);
                break;
            case RMH_EVENT_MOCA_RESET:
                RMH_PrintMsgT(&cbE->eventTime, "[*RESET* ] MoCA Reset triggered - %s\n", RMH_MoCAResetReasonToString(cbE->eventData.RMH_EVENT_MOCA_RESET.reason));
                break;
            case RMH_EVENT_MOCA_VERSION_CHANGED:
                app->appPrefix="[MOCA VER] ";
                printStatus|=RMHMonitor_Event_NetworkMoCAVersionChanged(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_MOCA_VERSION_CHANGED.version);
                break;
            case RMH_EVENT_NODE_JOINED:
                app->appPrefix="[++JOIN++] ";
                printStatus|=RMHMonitor_Event_JoinNode(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_NODE_JOINED.nodeId);
                break;
            case RMH_EVENT_NODE_DROPPED:
                app->appPrefix="[--DROP--] ";
                printStatus|=RMHMonitor_Event_DropNode(app, &cbE->eventTime, cbE->eventData.RMH_EVENT_NODE_JOINED.nodeId);
                break;
            case RMH_EVENT_NC_ID_CHANGED:
                if (cbE->eventData.RMH_EVENT_NC_ID_CHANGED.ncValid) {
                    app->appPrefix="[NC CHNGE] ";
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
}
