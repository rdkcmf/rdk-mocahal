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
#ifndef RMH_APP_H
#define RMH_APP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rdk_moca_hal.h"

#define RMH_PrintErr(fmt, ...)      RMH_Print(RMH_LOG_ERROR,   "ERROR: ", fmt, ##__VA_ARGS__);
#define RMH_PrintWrn(fmt, ...)      RMH_Print(RMH_LOG_ERROR,   "WARNING: ", fmt, ##__VA_ARGS__);
#define RMH_PrintMsg(fmt, ...)      RMH_Print(RMH_LOG_MESSAGE, "", fmt, ##__VA_ARGS__);
#define RMH_PrintDbg(fmt, ...)      RMH_Print(RMH_LOG_DEBUG,   "", fmt, ##__VA_ARGS__);
#define RMH_Print(level, logPrefix, fmt, ...) { \
    if (app && (app->apiLogLevel & level) == level) { \
        fprintf(stdout, "%s" logPrefix  fmt "", app->appPrefix ? app->appPrefix : "", ##__VA_ARGS__); \
    } \
}

struct RMHApp;
typedef struct RMHApp_API {
    const char* apiName;
    const char* apiAlias;
    RMH_Result (*apiFunc)();
    const RMH_Result (*apiHandlerFunc)(const struct RMHApp *app, void *api);
} RMHApp_API;

typedef struct RMHApp_List {
    uint32_t apiListSize;
    struct RMHApp_API apiList[RMH_MAX_NUM_APIS];
} RMHApp_List;

typedef struct RMHApp {
    RMH_Handle rmh;
    int argc;
    char **argv;

    bool argMonitorDriverDebug;
    bool argHelpRequested;
    bool argPrintMatch;
    bool argPrintApis;

    uint32_t apiLogLevel;
    uint32_t driverLogLevel;
    const char* argRunCommand;
    const char* appPrefix;

    RMH_APIList* allAPIs;
    RMH_APIList* unimplementedAPIs;
    RMH_APITagList* rmhAPITags;

    RMHApp_List handledAPIs;
    RMH_APIList local;
} RMHApp;

RMH_Result RMHApp_ReadMenuOption(RMHApp *app, uint32_t *value, bool helpSupported, bool *helpRequested);
inline const char * RMHApp_ReadNextArg(RMHApp *app);
void RMHApp_RegisterAPIHandlers(RMHApp *app);

#endif
