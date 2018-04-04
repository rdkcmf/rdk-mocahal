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

/**
 * Declaring this global so we don't need to come up with 2K on the stack on every print. Could move this to 'app' in the future if we want.
 */
#define PRINTBUF_SIZE 2048
static char printBuff[PRINTBUF_SIZE];

inline
void RMH_Write(RMHMonitor *app, const char*buf) {
    fputs(buf, app->out_file ? app->out_file : stdout);
}

/**
 * This is the main print function in this program. It will attempt to write 'fmt' string to stdout. It will also account for
 * new lines and print the time and appPrefix before each line
 */
void RMH_Print_Raw(RMHMonitor *app, struct timeval *time, const char*fmt, ...) {
    struct tm* tm_info;
    char *printString=printBuff;
    int printSize;
    va_list ap;
    static bool newLine = true;
    int i = 0;

    /* First construct the line to print and get the size */
    va_start(ap, fmt);
    printSize = vsnprintf(printString, PRINTBUF_SIZE, fmt, ap);
    va_end(ap);

    /* Make sure we have something valid to print */
    if (printSize > 0) {
        char timeBuf[64];

        /* We have something to print, check if we need to also be printing a timestamp */
        if (app->printTimestamp) {
            /* Timestamp is needed, convert the timestamp we were passed to a readable string */
            tm_info = localtime(&time->tv_sec);
            snprintf(timeBuf, sizeof(timeBuf), "%02d.%02d.%02d %02d:%02d:%02d:%03ld ", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, time->tv_usec/1000);
        }

#if 0
        /* If there is a specificed prefix add it to this timeBuf. Then we need only worry about a single prefix */
        if (app->appPrefix && sizeof(timeBuf) > strlen(app->appPrefix) + strlen(timeBuf)) {
            strcat(timeBuf, app->appPrefix);
        }
#endif

        /* Walk through the entire string we were given. Only way to check for new lines */
        while(printString[i] != '\0') {
            if (newLine) {
                /* This is a new line, start by printing the timestamp and prefix */
                if (app->printTimestamp) RMH_Write(app, timeBuf);
                if (app->appPrefix) RMH_Write(app, app->appPrefix);
                newLine=false;
            }

            if (printString[i] == '\n') {
                /* We found a new line. Replace the next character with a temporary terminator and
                   print the string out. After printing, put the replaced character back where we
                   found it. Note, if this is the last line we'll just be replacing \0 with another
                   \0 so it should be safe */
                char tmp=printString[i+1];
                printString[i+1]='\0';
                RMH_Write(app, printString);
                printString[i+1]=tmp;

                /* Move the start of the string forward to after the newline and set the newline flag */
                printString=&printString[i+1];
                newLine=true;
                i=0;
            }
            else {
                /* Not a newline, move to the next character */
                i++;
            }
        }

        /* We've found the end of the string. If there some string remaining, print it */
        if (i > 0)
            RMH_Write(app, printString);

        /* If we were limited by our buffer size inform the user. We still need to reset newline here since it's static */
        if (printSize >= PRINTBUF_SIZE) {
            RMH_Write(app, "\nLog message truncated\n");
            newLine=true;
        }
    }
    else if (printSize < 0 ) {
        /* This should never happen. But if it does we still need to reset newline here since it's static  */
        RMH_Write(app, "\nInternal print error\n");
        newLine=true;
    }

    fflush(stdout);
}