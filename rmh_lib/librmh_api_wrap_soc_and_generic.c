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

RMH_Result GENERIC_IMPL__RMH_Destroy(const RMH_Handle handle) {
    if (handle->soclib) {
        dlclose(handle->soclib);
    }
    if (handle->printBuf) {
        free(handle->printBuf);
    }
    if (handle) {
        free(handle);
    }
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_Log_SetAPILevel(const RMH_Handle handle, const RMH_LogLevel value){
    handle->logLevelBitMask=value;
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_Log_GetAPILevel(const RMH_Handle handle, RMH_LogLevel* response) {
    *response=handle->logLevelBitMask;
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_SetEventCallbacks(const RMH_Handle handle, const uint32_t value) {
    handle->eventNotifyBitMask=value;
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_GetEventCallbacks(const RMH_Handle handle, uint32_t* response) {
    *response=handle->eventNotifyBitMask;
    return RMH_SUCCESS;
}
