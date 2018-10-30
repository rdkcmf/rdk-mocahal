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

#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>
#include "rmh_monitor.h"


static
RMH_Result RMHMonitor_PrintUsage(RMHMonitor *app) {
    RMH_PrintMsg("usage: rmh_monitor [-?|-h|--help] [-n|--no_timestamp] [-t|--trace]\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("   --help            Print this help message\n");
    RMH_PrintMsg("   --no_timestamp    Do not prefix each line with a timestamp\n");
    RMH_PrintMsg("   --debug           Enable API trace messages\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("\n");
    return RMH_SUCCESS;
}


/**
 * This function handles all incomming callbacks from RMH. If they are print events we will handle them right away.
 * All other callbacks will be enqueued to be handled by the event thread.
*/
static
void RMHMonitor_RMHCallback(const enum RMH_Event event, const struct RMH_EventData *eventData, void* userContext){
    RMHMonitor *app=(RMHMonitor *)userContext;

    switch(event) {
    case RMH_EVENT_API_PRINT:
        RMH_PrintMsg("%s", eventData->RMH_EVENT_API_PRINT.logMsg);
        break;
    case RMH_EVENT_DRIVER_PRINT:
        RMH_PrintMsg("%s", eventData->RMH_EVENT_DRIVER_PRINT.logMsg);
        break;
    default:
        RMHMonitor_Queue_Enqueue(app, event, eventData);
        break;
    }
}


/**
 * Handle all arguments passed into the application. Arguments should update values in 'app' and then used later.
*/
static
RMH_Result RMHMonitor_ParseArguments(RMHMonitor *app, const int argc, const char *argv[]) {
    uint32_t i=1; /* start at one to skip program name */

    while (i<argc) {
        const char* option = argv[i];
        if (!option) break;

        if (option[0] == '-') {
            if (strcmp(option, "-t") == 0 || strcmp(option, "--trace") == 0) {
                app->apiLogLevel |= RMH_LOG_TRACE;
            } else if (strcmp(option, "-n") == 0 || strcmp(option, "--no_timestamp") == 0) {
                app->noPrintTimestamp = true;
            } else if (strcmp(option, "-?") == 0 || strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
                return RMH_FAILURE;
            } else {
                RMH_PrintWrn("Unknown option '%s'! Skipping\n", option);
                return RMH_FAILURE;
            }
        }
        else {
            RMH_PrintWrn("Unknown argument '%s'! Skipping\n", option);
            return RMH_FAILURE;
        }
        i++;
    };

    return RMH_SUCCESS;
}


/***********************************************************
 * Main
 ***********************************************************/
int main(const int argc, const char *argv[])
{
    RMHMonitor appStr;
    RMHMonitor* app=&appStr;
    RMH_Result result=RMH_FAILURE;

    /* Initialize everything to defaults */
    memset(app, 0, sizeof(*app));
    app->apiLogLevel = RMH_LOG_DEFAULT;

    /* Parse arguments and update options in app */
    if (RMHMonitor_ParseArguments(app, argc, argv) != RMH_SUCCESS) {
        RMHMonitor_PrintUsage(app);
        exit(0);
    }

    /* Connect to RMH */
    app->rmh=RMH_Initialize(RMHMonitor_RMHCallback, app);
    if (!app->rmh){
        RMH_PrintErr("Failed in RMH_Initialize!\n");
        goto exit_err;
    }

    /* Notify what logging level we want */
    if (RMH_Log_SetAPILevel(app->rmh, app->apiLogLevel ) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set the log level!\n");
    }

    /* Setup all the callbacks we're interested in...pretty much everything */
    if (RMH_SetEventCallbacks(app->rmh, RMH_EVENT_LINK_STATUS_CHANGED | \
                                        RMH_EVENT_MOCA_RESET | \
                                        RMH_EVENT_MOCA_VERSION_CHANGED | \
                                        RMH_EVENT_ADMISSION_STATUS_CHANGED | \
                                        RMH_EVENT_NODE_JOINED | \
                                        RMH_EVENT_NODE_DROPPED | \
                                        RMH_EVENT_NC_ID_CHANGED | \
                                        RMH_EVENT_LOW_BANDWIDTH | \
                                        RMH_EVENT_API_PRINT) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set event callbacks!\n");
        goto exit_err_init;
    }

    /* Initialize the event queue. The event queue and thread allow us to get out of the callbacks ASAP */
    TAILQ_INIT(&app->eventQueue);
    app->eventNotify=RMHMonitor_Semaphore_Create();
    if (!app->eventNotify) {
        RMH_PrintErr("Failed RMHMonitor_Semaphore_Create!\n");
        goto exit_err_init;
    }

    /* Setup the event notify semaphore */
    RMHMonitor_Semaphore_Reset(app->eventNotify);

    /* Initialize the threads mutex */
    if (pthread_mutex_init(&app->eventQueueProtect, NULL) != 0) {
        RMH_PrintErr("Failed to create eventQueueProtect mutex!\n");
        goto exit_err_eventNotify;
    }

    /* Mark the thread as running and create it */
    app->eventThreadRunning=true;
    if (pthread_create(&app->eventThread, NULL, (void *)RMHMonitor_Event_Thread, app) != 0) {
        RMH_PrintErr("Failed creating event thread!");
        goto exit_err_mutex;
    }
    else {
        RMH_PrintMsg("Begin monitoring MoCA status. Press enter to exit...\n");

        /* Wait for a good reason to exit */
        fgetc(stdin);
        RMH_PrintMsg("Exit status monitor\n");

        /* Mark the thread as stopped */
        app->eventThreadRunning=false;

        /* Kick the thread so it can exit */
        RMHMonitor_Semaphore_Signal(app->eventNotify);

        /* Exit the pthread */
        if(pthread_join(app->eventThread, NULL)) {
            RMH_PrintErr("Failed doing pthread_join");
        }
    }

    /* We made it to the end with no filures, indicate as such */
    result=RMH_SUCCESS;

exit_err_mutex:
    /* Shutdown the event queue */
    pthread_mutex_destroy (&app->eventQueueProtect);

exit_err_eventNotify:
    /* Delete the semaphore */
    if (app->eventNotify) {
        RMHMonitor_Semaphore_Destroy(app->eventNotify);
        app->eventNotify = NULL;
    }

exit_err_init:
    /* Disconnect from RMH */
    RMH_Destroy(app->rmh);

exit_err:
    return result;
}
