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

#ifndef LIB_RMH_H
#define LIB_RMH_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include "rmh_type.h"

void RMH_Print(const RMH_Handle handle, const RMH_LogLevel level, const char *filename, const uint32_t lineNumber, const char *format, ...);
#define RMH_PrintErr(fmt, ...)      if (!handle || (handle->logLevelBitMask & RMH_LOG_ERROR) == RMH_LOG_ERROR)      { RMH_Print(handle, RMH_LOG_ERROR, __FUNCTION__, __LINE__, "ERROR: " fmt, ##__VA_ARGS__); }
#define RMH_PrintWrn(fmt, ...)      if (!handle || (handle->logLevelBitMask & RMH_LOG_WARNING) == RMH_LOG_WARNING)  { RMH_Print(handle, RMH_LOG_WARNING, __FUNCTION__, __LINE__, "WARNING: " fmt, ##__VA_ARGS__); }
#define RMH_PrintMsg(fmt, ...)      if (!handle || (handle->logLevelBitMask & RMH_LOG_MESSAGE) == RMH_LOG_MESSAGE)  { RMH_Print(handle, RMH_LOG_MESSAGE, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); }
#define RMH_PrintDbg(fmt, ...)      if (!handle || (handle->logLevelBitMask & RMH_LOG_DEBUG) == RMH_LOG_DEBUG)      { RMH_Print(handle, RMH_LOG_DEBUG, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); }
#define RMH_PrintTrace(fmt, ...)    if ((handle->logLevelBitMask & RMH_LOG_TRACE) == RMH_LOG_TRACE)                 { RMH_Print(handle, RMH_LOG_TRACE, __FUNCTION__, __LINE__, "[0x%p|0x%p]%.*s" fmt, handle, handle->handle, handle->apiDepth, "\t\t\t\t\t\t\t\t\t", ##__VA_ARGS__); }

#define BRMH_RETURN_IF(expr, ret) { if (expr) { RMH_PrintErr("'" #expr "' is true!\n"); return ret; } }
#define BRMH_RETURN_IF_FAILED(cmd) { \
    RMH_Result ret = cmd; \
    if (ret != RMH_SUCCESS) { \
        RMH_PrintDbg("'" #cmd "' failed with error %s!\n", RMH_ResultToString(ret)); \
        return ret; \
    } \
}

#define RMH_MAX_PRINT_LINE_SIZE 2048

extern RMH_APIList hRMHGeneric_APIList;
extern RMH_APIList hRMHGeneric_SoCUnimplementedAPIList;
extern RMH_APITagList hRMHGeneric_APITags;

typedef struct RMH {
    RMH_Handle handle;
    void* soclib;
    FILE *localLogToFile;
    struct timeval startTime;
    RMH_EventCallback eventCB;
    char* printBuf;
    RMH_LogLevel logLevelBitMask;
    RMH_Event eventNotifyBitMask;
    void* eventCBUserContext;
    uint32_t apiDepth;
} RMH;

#endif /* LIB_RMH_H */
