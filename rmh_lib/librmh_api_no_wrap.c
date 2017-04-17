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

#include "librmh.h"
#include "rdk_moca_hal.h"

RMH_Handle RMH_Initialize(const RMH_EventCallback eventCB, void* userContext) {
    RMH* handle;

    handle=malloc(sizeof(*handle));
    BRMH_RETURN_IF(handle==NULL, NULL);
    memset(handle, 0, sizeof(*handle));
    handle->printBuf=malloc(RMH_MAX_PRINT_LINE_SIZE);
    handle->logLevelBitMask=RMH_LOG_ERROR;
    if (!handle->printBuf) {
        RMH_PrintErr("Unable to allocate print buf of size %u!\n", RMH_MAX_PRINT_LINE_SIZE);
        goto error_out;
    }
    handle->eventCB=eventCB;
    handle->eventCBUserContext=userContext;
    handle->soclib = dlopen("librdkmocahalsoc.so.0", RTLD_LAZY);
    if (handle->soclib) {
        RMH_Handle (*apiFunc)() = dlsym(handle->soclib, "RMH_Initialize");
        if (dlerror()) return NULL;
        if (!apiFunc) return NULL;
        handle->handle = apiFunc(eventCB, userContext);
        if (!handle->handle) {
            RMH_PrintErr("Failed when initializing the SoC MoCA Hal!\n");
            goto error_out;
        }
    }
    else {
        RMH_PrintWrn("Failed to open the SoC library 'librdkmocahalsoc.so.0', all MoCA APIs will be unimplemented!  Please ensure it's properly installe don this system\n");
    }
    RMH_PrintTrace("Initialized generic handle:0x%p socHandle:0x%p\n", handle, handle->handle);
    return (RMH_Handle)handle;

error_out:
    RMH_Destroy(handle);
    return NULL;
}


#undef AS
#define AS(x,y) #x,
__attribute__((visibility("hidden"))) const char * const RMH_ResultStr[] = { ENUM_RMH_Result };
__attribute__((visibility("hidden"))) const char * const RMH_LinkStatusStr[] = { ENUM_RMH_LinkStatus };
__attribute__((visibility("hidden"))) const char * const RMH_PowerModeStr[] = { ENUM_RMH_PowerMode };
__attribute__((visibility("hidden"))) const char * const RMH_MoCAVersionStr[] = { ENUM_RMH_MoCAVersion };
__attribute__((visibility("hidden"))) const char * const RMH_LogLevelStr[] = { ENUM_RMH_LogLevel };
__attribute__((visibility("hidden"))) const char * const RMH_EventStr[] = { ENUM_RMH_Event };

const char* const RMH_ResultToString(const RMH_Result value) {
    return RMH_ResultStr[value];
}

const char* const RMH_LinkStatusToString(const RMH_LinkStatus value) {
    return RMH_LinkStatusStr[value];
}

const char* const RMH_MoCAVersionToString(const RMH_MoCAVersion value) {
    switch (value) {
        case RMH_MOCA_VERSION_UNKNOWN:
            return "Unknown";
            break;
        case RMH_MOCA_VERSION_10:
            return "1.0";
            break;
        case RMH_MOCA_VERSION_11:
            return "1.1";
            break;
        case RMH_MOCA_VERSION_20:
            return "2.0";
            break;
        default:
            return RMH_MoCAVersionStr[value];
            break;
    }
}

static inline
const char* const RMH_BitMAskToString(const uint32_t value, char* responseBuf, const size_t responseBufSize, const char * const fixedStrings[], const uint32_t numFixedStrings) {
    if (responseBuf) {
        int i;
        responseBuf[0]='\0';
        for(i=0; i < numFixedStrings; i ++) {
            if ((value & (1u<<i)) == (1u<<i)) {
                if (responseBufSize <= strlen(responseBuf) + strlen(fixedStrings[i]) + 3) {
                    return "Buffer too small!";
                }
                if (strlen(responseBuf)) {
                    strcat(responseBuf, " | ");
                }
                strcat(responseBuf, fixedStrings[i]);
            }
        }
    }
    return responseBuf;
}

const char* const RMH_PowerModeToString(const uint32_t value, char* responseBuf, const size_t responseBufSize) {
    return RMH_BitMAskToString(value, responseBuf, responseBufSize, RMH_PowerModeStr, sizeof(RMH_PowerModeStr)/sizeof(RMH_PowerModeStr[0]));
}

const char* const RMH_LogLevelToString(const uint32_t value, char* responseBuf, const size_t responseBufSize) {
    return RMH_BitMAskToString(value, responseBuf, responseBufSize, RMH_LogLevelStr, sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0]));
}

const char* const RMH_EventToString(const uint32_t value, char* responseBuf, const size_t responseBufSize) {
    return RMH_BitMAskToString(value, responseBuf, responseBufSize, RMH_EventStr, sizeof(RMH_EventStr)/sizeof(RMH_EventStr[0]));
}

const char* const RMH_MacToString(const RMH_MacAddress_t mac, char* responseBuf, const size_t responseBufSize) {
    if (responseBuf && responseBufSize >= 18 ) {
        int written=snprintf(responseBuf, responseBufSize, "%02X:%02X:%02X:%02X:%02X:%02X",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        if ((written > 0) && (written < responseBufSize))
            return responseBuf;
    }
    return "Buffer too small!";
}

