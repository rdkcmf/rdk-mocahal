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

static
__attribute__((constructor(200))) void pRMH_APIWRAP_Initialize() {
    strncpy( hRMHGeneric_APIList.apiListName, "All APIs", sizeof(hRMHGeneric_APIList.apiListName));
    hRMHGeneric_APIList.apiListSize=0;

    strncpy( hRMHGeneric_SoCUnimplementedAPIList.apiListName, "Unimplemented APIs", sizeof(hRMHGeneric_SoCUnimplementedAPIList.apiListName));
    hRMHGeneric_SoCUnimplementedAPIList.apiListSize=0;
    hRMHGeneric_APITags.tagListSize=0;
}

int pRMH_APIWRAP_Compare(const void* a, const void* b) {
    const RMH_API* _a=* (RMH_API * const *)a;
    const RMH_API* _b=* (RMH_API * const *)b;
    return strcasecmp(_a->apiName, _b->apiName);
}

static
__attribute__((constructor(400))) void pRMH_APIWRAP_Finalize() {
    /* Sort the list to ensure consistant ordering */
    qsort(hRMHGeneric_APIList.apiList, hRMHGeneric_APIList.apiListSize, sizeof(RMH_API*), pRMH_APIWRAP_Compare);
}

static inline
void pRMH_APIWRAP_PreAPIExecute(const RMH_Handle handle, const char *api, const bool socEnabled, const bool socBeforeGeneric) {
    handle->apiDepth++;
    RMH_PrintTrace("+++++ Enter %s [%s] ++++\n", api, !socEnabled ? "Generic Only" :
                                                                    socBeforeGeneric ? "SoC Before Generic" : "SoC After Generic");
    gettimeofday(&handle->startTime, NULL);
}

static inline
RMH_Result pRMH_APIWRAP_PostAPIExecute(const RMH_Handle handle, const char *api, const RMH_Result genRet, const RMH_Result socRet) {
    double elapsedTime;
    struct timeval stopTime;
    RMH_Result ret = (socRet == RMH_SUCCESS) ? genRet : socRet;
    gettimeofday(&stopTime, NULL);
    elapsedTime = (stopTime.tv_sec - handle->startTime.tv_sec) * 1000.0; /* sec to ms */
    elapsedTime += (stopTime.tv_usec - handle->startTime.tv_usec) / 1000.0; /* us to ms */
    RMH_PrintTrace("------ Exit  %s [%s] -- [Time: %.02fms] ----\n", api, RMH_ResultToString(ret), elapsedTime);
    handle->apiDepth--;
    return ret;
}

static inline
RMH_Result pRMH_APIWRAP_GetSoCAPI(const RMH_Handle handle, const char *apiName, RMH_Result (**apiFunc)()) {
    RMH_Result ret=RMH_UNIMPLEMENTED;
    if (handle->soclib) {
        char *dlErr = dlerror(); /* Check error first to clear it out */
        *apiFunc=dlsym(handle->soclib, apiName);
        dlErr = dlerror();
        if (dlErr) {
            RMH_PrintTrace("Error for '%s' in dlopen: '%s'\n", apiName, dlErr);
        }
        else if (*apiFunc) {
            RMH_PrintTrace("Located SoC API '%s' at 0x%p\n", apiName, *apiFunc);
            if (handle->handle) {
                ret = RMH_SUCCESS;
            } else {
                RMH_PrintTrace("Located SoC API '%s' at 0x%p but SoC library is not initialized!\n", apiName, *apiFunc);
                ret = RMH_INVALID_INTERNAL_STATE;
            }
        }
        else {
            RMH_PrintTrace("Unable to find SoC implementation of '%s'\n", apiName);
        }
    }
    return ret;
}

static inline
void pRMH_APIWRAP_RegisterAPI(RMH_API *api) {
    if (hRMHGeneric_APIList.apiListSize >= RMH_MAX_NUM_APIS) {
        fprintf(stderr, "ERROR: Unable to expose API %s! Increase the size of RMH_MAX_NUM_APIS\n", api->apiName);
        return;
    }
    hRMHGeneric_APIList.apiList[hRMHGeneric_APIList.apiListSize++]=api;
}



/**********************************************************************************************************************
The meaning of each of these RMH_API_IMPLEMENTATION macros is listed in rdk_moca_hal_types. For our purpose herem these
just redirect to __RMH_WRAP_API() if an API wrapper is to be defined. The first three parameters passed to
__RMH_WRAP_API() indicate how it should call the APIs.

In the case of RMH_API_IMPLEMENTATION_NO_WRAP() no wrapper is requred so it maps directly to __RMH_REGISTER_API().
**********************************************************************************************************************/
#undef RMH_API_IMPLEMENTATION_SOC_ONLY
#undef RMH_API_IMPLEMENTATION_GENERIC_ONLY
#undef RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC
#undef RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC

#define RMH_API_IMPLEMENTATION_SOC_ONLY(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RMH_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, false, true, false);

#define RMH_API_IMPLEMENTATION_GENERIC_ONLY(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RMH_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, true, false, false);

#define RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RMH_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, true, true, true);

#define RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RMH_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, true, true, false);

#include "librmh_wrap.h"

/* Reinclude API header to use the redefined macros to setup necessary functions and structs */
#undef RMH_API_H
#include "rmh_api.h"
