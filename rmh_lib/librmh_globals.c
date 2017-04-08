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
#include "librmh.h"
#include "rdk_moca_hal.h"

__attribute__((visibility("hidden"))) RMH_APIList hRMHGeneric_APIList;
__attribute__((visibility("hidden"))) RMH_APIList hRMHGeneric_SoCUnimplementedAPIList;
__attribute__((visibility("hidden"))) RMH_APITagList hRMHGeneric_APITags;

void RMH_Print(const RMH_Handle handle, const RMH_LogLevel level, const char *filename, const uint32_t lineNumber, const char *format, ...) {
    va_list args;
    va_start(args, format);
    if (handle && handle->localLogToFile) {
        vfprintf(handle->localLogToFile, format, args);
    }
    else if (handle && handle->printBuf && handle->eventCB && ((handle->eventNotifyBitMask & RMH_EVENT_API_PRINT) == RMH_EVENT_API_PRINT)) {
        RMH_EventData eventData;
        vsnprintf(handle->printBuf, RMH_MAX_PRINT_LINE_SIZE, format, args);
        eventData.RMH_EVENT_API_PRINT.logLevel=level;
        eventData.RMH_EVENT_API_PRINT.logMsg=(const char *)handle->printBuf;
        handle->eventCB(RMH_EVENT_API_PRINT, &eventData, handle->eventCBUserContext);
    }
    else {
        vprintf(format, args);
    }
    va_end(args);
}