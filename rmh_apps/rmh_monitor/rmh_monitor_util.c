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

#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "rmh_monitor.h"

RMHMonitor_hSemaphore RMHMonitor_Semaphore_Create() {
    RMHMonitor_hSemaphore eventHandle;

    eventHandle=malloc(sizeof(*eventHandle));
    if (!eventHandle) return NULL;
    memset(eventHandle, 0, sizeof(eventHandle));

    if (pthread_mutex_init(&eventHandle->lock, NULL) != 0) {
        free(eventHandle);
        return NULL;
    }
    if (pthread_cond_init(&eventHandle->cond, NULL) != 0) {
        pthread_mutex_destroy (&eventHandle->lock);
        free(eventHandle);
        return NULL;
    }
    eventHandle->signal = false;
    return eventHandle;
}

void RMHMonitor_Semaphore_Destroy(RMHMonitor_hSemaphore eventHandle) {
    if (eventHandle) {
        pthread_mutex_destroy (&eventHandle->lock);
        pthread_cond_destroy (&eventHandle->cond);
        free(eventHandle);
    }
}

RMH_Result RMHMonitor_Semaphore_Reset(RMHMonitor_hSemaphore eventHandle) {
    pthread_mutex_lock(&eventHandle->lock);
    eventHandle->signal = false ;
    pthread_mutex_unlock(&eventHandle->lock);
    return RMH_SUCCESS;
}

RMH_Result RMHMonitor_Semaphore_Signal(RMHMonitor_hSemaphore eventHandle) {
    pthread_mutex_lock(&eventHandle->lock);
    eventHandle->signal = true;
    pthread_cond_signal(&eventHandle->cond);
    pthread_mutex_unlock(&eventHandle->lock);
    return RMH_SUCCESS;
}

RMH_Result RMHMonitor_Semaphore_WaitTimeout(RMHMonitor_hSemaphore eventHandle, const int timeoutMsec) {
   struct timeval now;
   struct timespec timeout;
   RMH_Result result=RMH_FAILURE;

    pthread_mutex_lock(&eventHandle->lock);
    if (eventHandle->signal) {
        result=RMH_SUCCESS;
        eventHandle->signal = false;
        goto exit;
    }
    do {
        gettimeofday(&now, NULL);
        timeout.tv_nsec = now.tv_usec * 1000 + (timeoutMsec%1000)*1000000;
        timeout.tv_sec = now.tv_sec + (timeoutMsec/1000);
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_nsec -=  1000000000;
            timeout.tv_sec ++;
        }

        int rc = pthread_cond_timedwait(&eventHandle->cond, &eventHandle->lock, &timeout);
        if (eventHandle->signal) {
            /* even if we timed out, if the signal was set, we succeed. this allows magnum to
            be resilient to large OS scheduler delays */
            result = RMH_SUCCESS;
            break;
        }
        if (rc==ETIMEDOUT) {
            result = RMH_TIMEOUT;
            goto exit;
        }
        else if(rc==EINTR) {
            continue;
        }
        if (rc!=0) {
            result = RMH_FAILURE;
            goto exit;
        }
    } while(!eventHandle->signal);  /* we might have been wokenup and then event has been cleared */

    eventHandle->signal = false;
exit:
    pthread_mutex_unlock(&eventHandle->lock);
    return result;
}


/**
 * This function will post to the event queue which will then be read by the event thread
*/
void RMHMonitor_Queue_Enqueue(RMHMonitor *app, const enum RMH_Event event, const struct RMH_EventData *eventData) {
    RMHMonitor_CallbackEvent *cbE = malloc(sizeof(RMHMonitor_CallbackEvent));
    char printBuff[128];

    if (cbE) {
        /* Enqueue the event and the time it occurred */
        gettimeofday(&cbE->eventTime, NULL);
        cbE->event = event;
        cbE->eventData = *eventData;

        /* Lock the queue before we modify it */
        pthread_mutex_lock(&app->eventQueueProtect);
        TAILQ_INSERT_HEAD(&app->eventQueue, cbE, entries);
        RMH_PrintDbg("%s[%u] ENQUEUED event '%s' in %p\n", __FUNCTION__, __LINE__, RMH_EventToString(cbE->event, printBuff, sizeof(printBuff)/sizeof(printBuff[0])), cbE);
        pthread_mutex_unlock(&app->eventQueueProtect);
        RMHMonitor_Semaphore_Signal(app->eventNotify);
    }
}


/**
 * This function will remove the oldest event from the event queue.
*/
void RMHMonitor_Queue_Dequeue(RMHMonitor *app) {
    RMHMonitor_CallbackEvent *cbE;
    char printBuff[128];

    /* Remove the tail of the queue. Lock the queue before we modify it */
    pthread_mutex_lock(&app->eventQueueProtect);
    cbE=app->eventQueue.tqh_first;
    if (cbE) {
        TAILQ_REMOVE(&app->eventQueue, app->eventQueue.tqh_first, entries);
        RMH_PrintDbg("%s[%u] DEQUEUED event '%s' in %p\n", __FUNCTION__, __LINE__, RMH_EventToString(cbE->event, printBuff, sizeof(printBuff)/sizeof(printBuff[0])), cbE);
        free(cbE);
    }
    pthread_mutex_unlock(&app->eventQueueProtect);
}
