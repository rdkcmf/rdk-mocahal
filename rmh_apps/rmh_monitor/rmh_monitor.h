#ifndef RMH_APP_H
#define RMH_APP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include "rdk_moca_hal.h"

/**
 * A simple semaphore based on pthread
 */
typedef struct RMHMonitor_Semaphore {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool signal;
} RMHMonitor_Semaphore, *RMHMonitor_hSemaphore;

/**
 * Contains all necessary information to describe a callback from RMH. This allows
 * us to handle the event on a seperate thread then where the callback occurred.
 */
typedef struct RMHMonitor_CallbackEvent {
  RMH_Event event;                                  /* The event that occurred */
  RMH_EventData eventData;                          /* Whatever event data was provided with the event. Because this event will be handled
                                                       on a different thread, if eventData is ever a pointer we will need to copy. */
  struct timeval eventTime;                         /* The time the event occurred */
  TAILQ_ENTRY(RMHMonitor_CallbackEvent) entries;    /* For queuing */
} RMHMonitor_CallbackEvent;


/**
 * Stores information about a given node. We store this interally it so if a node leaves the network we still know something about it.
 */
typedef struct RMHMonitor_NodeInfo {
    bool joined;                                    /* Set to true if this node has actively joined the network */
    bool selfNode;                                  /* Set to true if this node is the local device */
    bool preferredNC;                               /* Set to true if this node is a preferred NC */
    RMH_MacAddress_t mac;                           /* The MAC address of the device */
    RMH_MoCAVersion mocaVersion;                    /* The highest version of MoCA supported by this node */
} RMHMonitor_NodeInfo;

/**
 * The current snapshot status of the network. We store this internally so we can check callbacks to ensure things have acutally changed
 * and we don't print duplicate messages.
 */
typedef struct RMHMonitor_NetworkStatus {
    RMH_MoCAVersion networkMoCAVer;                 /* The version of MoCA being used by the Network */
    bool networkMoCAVerValid;                       /* Set to true if the value of 'networkMoCAVer' is valid */
    uint32_t ncNodeId;                              /* The node ID of the NC */
    bool ncNodeIdValid;                             /* Set to true if the value of 'ncNodeId' is valid */
    uint32_t selfNodeId;                            /* The node ID of the local device */
    bool selfNodeIdValid;                           /* Set to true if the value of 'selfNodeId' is valid */
    RMHMonitor_NodeInfo self;                       /* Node info for the local device */
    RMHMonitor_NodeInfo nodes[RMH_MAX_MOCA_NODES];  /* Node info for all remote devices */
} RMHMonitor_NetworkStatus;


/**
 * This is the main sturcture of the applicaiton and contains everything needing to be shared between functions.
 */
typedef struct RMHMonitor {
    RMH_Handle rmh;                                 /* The handle to RMH for all MoCA calls */

    pthread_t eventThread;                          /* The thread which handles the MoCA events */
    bool eventThreadRunning;                        /* Must remain set to true to continue monitoring. If this goes to false the thread will exit. */
    TAILQ_HEAD(tailhead, RMHMonitor_CallbackEvent) eventQueue;
                                                    /* The queue where events are stored while they are moved to the event thread */
    pthread_mutex_t eventQueueProtect;              /* A mutex to ensure safe enqueue/dequeue of events */
    RMHMonitor_hSemaphore eventNotify;              /* A semaphore to notify the event thread there is work to be done */

    bool mocaEnabled;                               /* Indicates if MoCA is enabled on this device */
    bool mocaEnabledValid;                          /* Set to true if the value of 'mocaEnabled' is valid */
    RMH_LinkStatus linkStatus;                      /* The current state of the MoCA link for this device */
    bool linkStatusValid;                           /* Set to true if the value of 'linkStatus' is valid */
    RMHMonitor_NetworkStatus netStatus;             /* The current state of the MoCA network */

    RMH_LogLevel apiLogLevel;                       /* The logging level to print from the app and RMH */
    RMH_LogLevel driverLogLevel;                    /* The logging level to print from the MoCA driver itself */
    const char* appPrefix;                          /* A fixed string to prepend to every line */
    bool noPrintTimestamp;                          /* Set to true to avoid prefixing a each output line with a timestamp */
} RMHMonitor;


/* Logging functions to be used throughout the application */
#define RMH_PrintErr(fmt, ...)          RMH_Print(app, NULL, RMH_LOG_ERROR,   "ERROR: ", fmt, ##__VA_ARGS__);
#define RMH_PrintWrn(fmt, ...)          RMH_Print(app, NULL, RMH_LOG_ERROR,   "WARNING: ", fmt, ##__VA_ARGS__);
#define RMH_PrintMsg(fmt, ...)          RMH_Print(app, NULL, RMH_LOG_MESSAGE, "", fmt, ##__VA_ARGS__);
#define RMH_PrintMsgT(time, fmt, ...)   RMH_Print(app, time, RMH_LOG_MESSAGE, "", fmt, ##__VA_ARGS__);
#define RMH_PrintDbg(fmt, ...)          RMH_Print(app, NULL, RMH_LOG_DEBUG,   "", fmt, ##__VA_ARGS__);

/* Low level logging fuctions which are not intended to be called directly */
void RMH_Print_Raw(RMHMonitor *app, struct timeval *time, const char*fmt, ...);
#define RMH_Print(app, time, level, logPrefix, fmt, ...) { \
    if (app && (app->apiLogLevel & level) == level) { \
        if (time) { \
            RMH_Print_Raw(app, time, fmt, ##__VA_ARGS__); \
        } \
        else { \
            struct timeval now; \
            gettimeofday(&now, NULL); \
            RMH_Print_Raw(app, &now, fmt, ##__VA_ARGS__); \
        } \
    } \
}



RMHMonitor_hSemaphore RMHMonitor_Semaphore_Create();
void RMHMonitor_Semaphore_Destroy(RMHMonitor_hSemaphore eventHandle);
RMH_Result RMHMonitor_Semaphore_Reset(RMHMonitor_hSemaphore eventHandle);
RMH_Result RMHMonitor_Semaphore_Signal(RMHMonitor_hSemaphore eventHandle);
RMH_Result RMHMonitor_Semaphore_WaitTimeout(RMHMonitor_hSemaphore eventHandle, const int timeoutMsec);
void RMHMonitor_Queue_Dequeue(RMHMonitor *app);
void RMHMonitor_Queue_Enqueue(RMHMonitor *app, const enum RMH_Event event, const struct RMH_EventData *eventData);


void RMHMonitor_Event_Thread(void * context);


#endif