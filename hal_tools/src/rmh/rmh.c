#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "rdk_moca_hal.h"

#define RMH_ENABLE_PRINT_ERR 1
#define RMH_ENABLE_PRINT_WRN 1
#define RMH_ENABLE_PRINT_LOG 1

#define RMH_PrintErr(fmt, ...) if (RMH_ENABLE_PRINT_ERR) { \
    if (app->printAPINames && app->activeApi) {fprintf(stderr, "%s: ERROR: " fmt "", app->activeApi ? app->activeApi : "", ##__VA_ARGS__);} \
    else                                      {fprintf(stderr, "ERROR: " fmt "", ##__VA_ARGS__);} \
}

#define RMH_PrintWrn(fmt, ...) if (RMH_ENABLE_PRINT_WRN) { \
    if (app->printAPINames && app->activeApi) {fprintf(stdout, "%s: WARNING: " fmt "", app->activeApi ? app->activeApi : "", ##__VA_ARGS__);} \
    else                                      {fprintf(stdout, "WARNING: " fmt "", ##__VA_ARGS__);} \
}

#define RMH_PrintLog(fmt, ...) if (RMH_ENABLE_PRINT_LOG) { \
    if (app->printAPINames && app->activeApi) {fprintf(stdout, "%s: " fmt "", app->activeApi ? app->activeApi : "", ##__VA_ARGS__);} \
    else                                      {fprintf(stdout, "" fmt "", ##__VA_ARGS__);} \
}

typedef struct RMHApp_APIList {
    char name[32];
    struct RMHApp_API *apiList[2048];
    uint32_t apiListSize;
} RMHApp_APIList;

typedef struct RMHApp {
    RMH_Handle rmh;
    bool interactive;
    bool monitorCallbacks;
    bool printAPINames;
    int argc;
    char **argv;
    const char *activeApi;
    struct RMHApp_APIList allAPIs;
    struct RMHApp_APIList *subLists[32];
    uint32_t numSubLists;
} RMHApp;

typedef struct RMHApp_API {
    const char *name;
    RMH_Result (*apiHandlerFunc)(const struct RMHApp *app, void *api);

    /* Because of dynamic symbol loading paramaters are not important here */
    RMH_Result (*apiFunc)();
} RMHApp_API;


/* This macro first verifies the API matches what the handler expects. This is important because we store both the api
   and handler as (void*) so this line makes sure we catch any type issues at compile time */
#define ADD_API(_handler, _api) { \
    while(0) _handler(NULL, _api); \
    RMHApp_AddAPI(app, #_api, _handler, _api); \
}


/***********************************************************
 * User input functions
 ***********************************************************/

static inline
const char * ReadNextArg(RMHApp *app) {
    const char *str=NULL;
    if (app->argc >= 1) {
        str=app->argv[0];
        app->argv=&app->argv[1];
        app->argc--;
    }
    return str;
}

static
uint32_t ReadLine(const char *prompt, RMHApp *app, char *buf, const uint32_t bufSize) {
    const char *input=NULL;
    if (app->interactive) {
        if (prompt)
            RMH_PrintLog("%s", prompt);
        input=fgets(buf, bufSize, stdin);
        if (input) {
            buf[strcspn(buf, "\n")] = '\0';
        }
    }
    else {
        input=ReadNextArg(app);
        if (input) {
            strncpy(buf, input, bufSize);
            buf[bufSize]='\0';
        }
    }

    return (input && strlen(input)) ? strlen(input) : 0;
}

static
RMH_Result ReadUint32(RMHApp *app, uint32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (number): ", app, input, sizeof(input))) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 10);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadMAC(RMHApp *app, uint8_t (*response)[6]) {
    char input[18];
    char val[6];
    char dummy;
    int i;
    if (ReadLine("Enter MAC address (XX:XX:XX:XX:XX:XX): ", app, input, sizeof(input)) == 17) {
        if(sscanf( input, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%c", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5], &dummy) == 6) {
            for( i = 0; i < 6; i++ ) (*response)[i] = (uint8_t)val[i];
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid MAC address in the for 'XX:XX:XX:XX:XX:XX':\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadLogLevel(RMHApp *app, uint32_t *value) {
    char input[32];
    int i;

    if (ReadLine("Enter your choice (number or string): ", app, input, sizeof(input))) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 10);
        if (end != input && *end == '\0' && errno != ERANGE &&
            *value>=0 && *value<sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0])) {
            return RMH_SUCCESS;
        }
        for(i=0; i!=sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0]); i++) {
            if (strcasecmp(input, RMH_LogLevelStr[i]) == 0) {
                *value=i;
                return RMH_SUCCESS;
            }
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid number or log level\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadInt32(RMHApp *app, int32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (number): ", app, input, sizeof(input))) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 10);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid signed number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadUint32Hex(RMHApp *app, uint32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (Hex): ", app, input, sizeof(input))) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 0);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadBool(RMHApp *app, bool *value) {
    char input[16];
    if (ReadLine("Enter your choice (TRUE,FALSE or 1,0): ", app, input, sizeof(input))) {
        if ((strcmp(input, "1") == 0) || (strcasecmp(input, "TRUE") == 0)) {
            *value=1;
            return RMH_SUCCESS;
        }
        else if ((strcmp(input, "0") == 0) || (strcasecmp(input, "FALSE") == 0)) {
            *value=0;
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please use only TRUE,FALSE or 1,0\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadString(RMHApp *app, char *buf, const uint32_t bufSize) {
    if (ReadLine("Enter your choice: ", app, buf, bufSize)) {
        return RMH_SUCCESS;
    }
    RMH_PrintErr("Bad input. Please provide a valid string.\n");
    return RMH_FAILURE;
}


/***********************************************************
 * Callbacks
 ***********************************************************/

static void EventCallback(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext){
    RMHApp *app=(RMHApp *)userContext;
    switch(event) {
    case LINK_STATUS_CHANGED:
        RMH_PrintLog("\n%p: Link status changed to %s\n", userContext, RMH_LinkStatusToString(eventData->LINK_STATUS_CHANGED.status));
        break;
    case MOCA_VERSION_CHANGED:
        RMH_PrintLog("\n%p: MoCA Version Changed to %s\n", userContext, RMH_MoCAVersionToString(eventData->MOCA_VERSION_CHANGED.version));
        break;
    case RMH_LOG_PRINT:
        RMH_PrintLog("%s", eventData->RMH_LOG_PRINT.logMsg);
        break;
    default:
        RMH_PrintWrn("Unhandled MoCA event %u!\n", event);
        break;
    }
}


/***********************************************************
 * API Handler Functions
 ***********************************************************/
static
RMH_Result OUT_UINT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%u\n", response);
    }
    return ret;
}

static
RMH_Result OUT_UINT32_HEX(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("0x%08x\n", response);
    }
    return ret;
}

static
RMH_Result OUT_INT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, int32_t* response)) {
    int32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%d\n", response);
    }
    return ret;
}

static
RMH_Result OUT_UINT32_ARRAY(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize, uint32_t* responseBufUsed)) {
    uint32_t responseBuf[256];
    uint32_t responseBufUsed;
    int i;

    RMH_Result ret = api(app->rmh, responseBuf, sizeof(responseBuf)/sizeof(responseBuf[0]), &responseBufUsed);
    if (ret == RMH_SUCCESS) {
        for (i=0; i < responseBufUsed; i++) {
            RMH_PrintLog("[%02u] %u\n", i, responseBuf[i]);
        }
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_UINT32(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t nodeId, uint32_t* response)) {
    uint32_t response=0;
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%u\n", response);
        }
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_INT32(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t nodeId, int32_t* response)) {
    int32_t response=0;
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%d\n", response);
        }
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_FLOAT(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t nodeId, float* response)) {
    float response=0;
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%.03f\n", response);
        }
    }
    return ret;
}

static
RMH_Result OUT_BOOL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, bool* response)) {
    bool response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%s\n", response ? "TRUE" : "FALSE");
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_BOOL(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t nodeId, bool* response)) {
    bool response=0;
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%s\n", response ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result OUT_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    RMH_Result ret = api(app->rmh, response, sizeof(response));
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%s\n", response);
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint8_t nodeId, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, response, sizeof(response));
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%s\n", response);
        }
    }
    return ret;
}


static
RMH_Result IN_RMH_ENUM(RMHApp *app, const char* const (*api)(const RMH_Result)) {
    RMH_Result enumIndex;
    RMH_Result ret=ReadUint32(app, (uint32_t*)&enumIndex);
    if (ret == RMH_SUCCESS) {
        const char * str = api(enumIndex);
        if (str) {
            RMH_PrintLog("%s\n", str);
        }
    }
    return ret;
}


static
RMH_Result OUT_MAC(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, uint8_t (*response)[6])) {
    uint8_t response[6];
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        char mac[24];
        RMH_PrintLog("%s\n", RMH_MacToString(response, mac, sizeof(mac)/sizeof(mac[0])));
    }
    return ret;
}

static
RMH_Result IN_MAC(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t value[6])) {
    uint8_t mac[6];
    RMH_Result ret=ReadMAC(app, &mac);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, mac);
        if (ret == RMH_SUCCESS) {
            char macStr[24];
            RMH_PrintLog("Success passing to %s\n", RMH_MacToString(mac, macStr, sizeof(macStr)/sizeof(macStr[0])));
        }
    }
    return ret;
}


static
RMH_Result IN_UINT32_OUT_MAC(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, const uint8_t nodeId, uint8_t (*response)[6])) {
    uint8_t response[6];
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            char mac[24];
            RMH_PrintLog("%s\n", RMH_MacToString(response, mac, sizeof(mac)/sizeof(mac[0])));
        }
    }
    return ret;
}

static
RMH_Result OUT_POWER_MODE(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_PowerMode* response)) {
    RMH_PowerMode response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%s\n", RMH_PowerModeToString(response));
    }
    return ret;
}

static
RMH_Result OUT_MOCA_VERSION(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%s\n", RMH_MoCAVersionToString(response));
    }
    return ret;
}

static
RMH_Result IN_UINT32_OUT_MOCA_VERSION(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint8_t nodeId, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("%s\n", RMH_MoCAVersionToString(response));
        }
    }
    return ret;
}

static
RMH_Result OUT_LOGLEVEL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_LogLevel* response)) {
    RMH_LogLevel response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("%s\n", RMH_LogLevelToString(response));
    }
    return ret;
}

static
RMH_Result OUT_UINT32_NODELIST(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NodeList_Uint32_t* response)) {
    RMH_NodeList_Uint32_t response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i;
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                RMH_PrintLog("NodeId:%u -- %u\n", i, response.nodeValue[i]);
            }
        }
    }
    return ret;
}

static
RMH_Result OUT_UINT32_NODEMESH(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response)) {
    RMH_NodeMesh_Uint32_t response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i, j;

        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                RMH_NodeList_Uint32_t *nl = &response.nodeValue[i];
                RMH_PrintLog("NodeId:%u\n", i);
                for (j = 0; j < RMH_MAX_MOCA_NODES; j++) {
                    if (i!=j && response.nodePresent[j]) {
                        RMH_PrintLog("   %u: %u\n", j, nl->nodeValue[j]);
                    }
                }
            RMH_PrintLog("\n");
            }
        }
    }
    return ret;
}

static
RMH_Result IN_BOOL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const bool value)) {
    bool value;
    RMH_Result ret=ReadBool(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing %s\n", value ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result IN_UINT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=ReadUint32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result IN_INT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const int32_t value)) {
    int32_t value;
    RMH_Result ret=ReadInt32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing %d\n", value);
        }
    }
    return ret;
}

static
RMH_Result IN_UINT32_HEX(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=ReadUint32Hex(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing 0x%08x\n", value);
        }
    }
    return ret;
}

static
RMH_Result IN_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const char* value)) {
    char input[256];
    RMH_Result ret=ReadString(app, input, sizeof(input));
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, input);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing %s\n", input);
        }
    }
    return ret;
}

static
RMH_Result IN_LOGLEVEL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const RMH_LogLevel response)) {
    RMH_LogLevel value;
    int i;

    for (i=0; app->interactive && i != sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0]); i++ ) {
        RMH_PrintLog("%d. %s\n", i, RMH_LogLevelStr[i]);
    }
    RMH_Result ret=ReadLogLevel(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintLog("Success passing %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result NO_PARAMS(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh)) {
    RMH_Result ret = api(app->rmh);
    if (ret == RMH_SUCCESS) {
        RMH_PrintLog("Success\n");
    }
    return ret;
}

static
RMH_Result LOCAL(RMHApp *app, RMH_Result (*api)(RMHApp* app)) {
    return api(app);
}


/***********************************************************
 * Generic Helper Functions
 ***********************************************************/

static RMHApp_API *
FindAPI(const char *apiName, RMHApp *app) {
    uint32_t i;
    uint32_t nonLocalOffset=0;
    uint32_t localOffset=0;
    if(strncasecmp(apiName, "RMH_", 4) != 0) {
        /* The user did not provide "RMH_" as part of the API name. Skip this when checking the list */
        nonLocalOffset = 4;
    }
    if(strncasecmp(apiName, "LOCAL_", 6) != 0) {
        /* The user did not provide "LOCAL_" as part of the API name. Skip this when checking the list */
        localOffset = 6;
    }

    for (i=0; i != app->allAPIs.apiListSize; i++) {
        const char *checkName=app->allAPIs.apiList[i]->name;
        checkName += (app->allAPIs.apiList[i]->apiHandlerFunc == (void*)LOCAL) ? localOffset : nonLocalOffset;
        if(strcasecmp(apiName, checkName) == 0) {
            return app->allAPIs.apiList[i];
        }
    }

  return NULL;
}

static RMHApp_APIList *
FindAPIList(const char *listName, RMHApp *app) {
    uint32_t i;
    for (i=0; i != app->numSubLists; i++) {
        if(strcmp(listName, app->subLists[i]->name) == 0) {
            return app->subLists[i];
        }
    }

  return NULL;
}


void DisplayList(RMHApp *app, const RMHApp_APIList *activeList) {
    uint32_t i;
    RMH_PrintLog("\n\n");
    if (activeList) {
        RMH_PrintLog("%02d. %s\n", 1, "Go Back");
        for (i=0; i != activeList->apiListSize; i++) {
            if (activeList->apiList[i]->apiFunc) {
                RMH_PrintLog("%02d. %s\n", i+2, activeList->apiList[i]->name);
            }
            else {
                RMH_PrintLog("%02d. %s [UNIMPLEMENTED]\n", i+2, activeList->apiList[i]->name);
            }
        }
    }
    else{
        RMH_PrintLog("%02d. %s\n", 1, "Exit");
        for (i=0; i != app->numSubLists; i++) {
            RMH_PrintLog("%02d. %s\n", i+2, app->subLists[i]->name);
        }
    }
}

RMH_Result DoInteractive(RMHApp *app) {
    uint32_t option;
    RMHApp_APIList *activeList=NULL;

    /* Since we're interactive, setup callback notifications */
    RMH_Callback_RegisterEvents(app->rmh, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED);

    DisplayList(app, activeList);
    while(true) {
        if (ReadUint32(app, &option)==RMH_SUCCESS) {
            if (option != 0) {
                if ( option == 1 ) {
                    if (!activeList) return RMH_FAILURE;
                    activeList=NULL;
                }
                else {
                    option-=2;
                    if (activeList && option<activeList->apiListSize) {
                        RMH_Result ret = RMH_UNIMPLEMENTED;
                        app->activeApi=activeList->apiList[option]->name;
                        RMH_PrintLog("----------------------------------------------------------------------------\n");
                        if (activeList->apiList[option]->apiFunc) {
                            ret=activeList->apiList[option]->apiHandlerFunc(app, activeList->apiList[option]->apiFunc);
                        }
                        if (ret != RMH_SUCCESS) {
                            RMH_PrintErr("Failed with error: %s!\n", RMH_ResultToString(ret));
                        }
                        RMH_PrintLog("----------------------------------------------------------------------------\n\n\n");
                        app->activeApi=NULL;
                        continue;
                    }
                    else if (!activeList && option < app->numSubLists ) {
                        activeList=app->subLists[option];
                        DisplayList(app, activeList);
                        continue;
                    }
                }
            }
            RMH_PrintErr("Invalid selection\n");
        }
        DisplayList(app, activeList);
    }
    return RMH_SUCCESS;
}

RMH_Result DoNonInteractive(RMHApp *app) {
    const char* option;

    /* First read to drop program name */
    ReadNextArg(app);

    do {
        option=ReadNextArg(app);
        if (!option) break;

        /* This is an option, not an API */
        if (strlen(option) == 2){
            switch (option[1]) {
            case 'm': app->monitorCallbacks = true; break;
            case 'n': app->printAPINames = true; break;
            default:
                RMH_PrintWrn("The option '%c' not supported!\n", option[1]);
                return RMH_UNIMPLEMENTED;
                break;
            }
        }
        else {
            /* not static, it must be an API call */
            RMHApp_API *api=FindAPI(option, app);
            if (api && api->apiFunc) {
                app->activeApi=api->name;
                RMH_Result ret=api->apiHandlerFunc(app, api->apiFunc);
                app->activeApi=NULL;
                if (ret != RMH_SUCCESS) {
                    return ret;
                }
            }
            else {
                RMH_PrintWrn("That API '%s'' is either not implemented or does not exist!\n", option);
                return RMH_UNIMPLEMENTED;
            }
        }
    } while(option);

    if (app->monitorCallbacks) {
        RMH_PrintLog("Monitoring for callbacks and MoCA logs. Press enter to exit...\n");
        fgetc(stdin);
    }
    return RMH_SUCCESS;
}

static inline
void RMHApp_AddAPI(RMHApp* app, const char* apiName, void *apiHandlerFunc, void *apiFunc) {
    char *lastChar;
    char subListName[32];
    RMHApp_APIList *subList;
    RMHApp_API *newItem=(RMHApp_API *)malloc(sizeof(RMHApp_API));
    memset(newItem, 0, sizeof(RMHApp_API));
    newItem->apiHandlerFunc=(void*)apiHandlerFunc;
    newItem->name=apiName;
    *(void **)(&newItem->apiFunc) = apiFunc;
    app->allAPIs.apiList[app->allAPIs.apiListSize++]=newItem;

    if ((strncmp(apiName,"RMH_",4) == 0) && ((lastChar = strchr(&apiName[4], '_')) != NULL)) {
        uint32_t copySize= (lastChar-&apiName[4]) > sizeof(subListName) ? sizeof(subListName) : (lastChar-&apiName[4]);
        strncpy(subListName, &apiName[4], copySize);
        subListName[copySize]='\0';
    }
    else if (strncmp(apiName,"LOCAL_",6) == 0) {
        strcpy(subListName, "App Functions");
    }
    else {
        strcpy(subListName, "Uncategorized");
    }

    /* If this API isn't implemented add it to a special list */
    if (!newItem->apiFunc) {
        subList=FindAPIList("Unimplemented", app);
        if (subList) {
            subList->apiList[subList->apiListSize++]=newItem;
        }
        else {
            int i;
            subList=(RMHApp_APIList *)malloc(sizeof(RMHApp_APIList));
            memset(subList, 0, sizeof(RMHApp_APIList));
            strcpy(subList->name, "Unimplemented");

            /* Ensure unimplemented list is at index 0 */
            for (i=app->numSubLists + 1; i > 0 ; i--) {
                app->subLists[i]=app->subLists[i-1];
            }
            app->subLists[0]=subList;
            subList->apiList[subList->apiListSize++]=newItem;
            app->numSubLists++;
        }
    }

    /* Add the API to it's sublist */
    subList=FindAPIList(subListName, app);
    if (subList) {
        subList->apiList[subList->apiListSize++]=newItem;
    }
    else {
        subList=(RMHApp_APIList *)malloc(sizeof(RMHApp_APIList));
        memset(subList, 0, sizeof(RMHApp_APIList));
        strcpy(subList->name, subListName);
        app->subLists[app->numSubLists++]=subList;
        subList->apiList[subList->apiListSize++]=newItem;
    }
}


/***********************************************************
 * Local API functions
 ***********************************************************/
static
RMH_Result LOCAL_log_stop(RMHApp* app) {
    char fileName[128];

    if (RMH_Log_GetFilename(app->rmh, fileName, sizeof(fileName)) == RMH_SUCCESS) {
        RMH_PrintLog("MoCA stopping logging to -- %s\n", fileName);

        if (RMH_Log_SetFilename(app->rmh, NULL) != RMH_SUCCESS) {
            RMH_PrintErr("Failed to stop logging!\n");
            return RMH_FAILURE;
        }
    }

    if (RMH_Log_SetLevel(app->rmh, RMH_LOG_LEVEL_DEFAULT) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set log level to RMH_LOG_LEVEL_DEBUG!\n");
        return RMH_FAILURE;
    }

    return RMH_SUCCESS;
}

static
RMH_Result LOCAL_log(RMHApp* app) {
    if (RMH_Log_SetLevel(app->rmh, RMH_LOG_LEVEL_DEBUG) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set log level to RMH_LOG_LEVEL_DEBUG!\n");
        return RMH_FAILURE;
    }

    return RMH_SUCCESS;
}

static
RMH_Result WriteVersionTxtToFile(RMHApp* app, const char* filename) {
    char line[1024];
    FILE *outFile;
    FILE *inFile;
    int i;
    outFile=fopen(filename, "a");
    if (outFile == NULL) {
        return RMH_FAILURE;
    }
    inFile=fopen("/version.txt", "r");
    if (outFile == NULL) {
        fclose(outFile);
        return RMH_FAILURE;
    }

    fprintf(outFile, "= Device Version information =================================================================================\n");
    for(i=0; i<10; i++) {
        if (fgets(line, sizeof(line), inFile) == NULL)
            break;
        fputs(line, outFile);
    }
    fprintf(outFile, "\n\n");

    fclose(outFile);
    fclose(inFile);

    return RMH_SUCCESS;
}

static
RMH_Result LOCAL_log_forever(RMHApp* app) {
    uint8_t mac[6];
    char fileName[128];
    struct tm* tm_info;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    if (RMH_Log_GetFilename(app->rmh, fileName, sizeof(fileName)) == RMH_SUCCESS) {
        RMH_PrintLog("MoCA stopping logging to -- %s\n", fileName);
    }

    if (RMH_Interface_GetMac(app->rmh, &mac) != RMH_SUCCESS) {
        memset(mac, 0, sizeof(mac));
    }

    snprintf(fileName, sizeof(fileName), "/opt/rmh_moca_log_%s_MAC_%02X-%02X-%02X-%02X-%02X-%02X_TIME_%02d.%02d.%02d_%02d-%02d-%02d.log",
#ifdef MACHINE_NAME
        MACHINE_NAME,
#else
        "unknown",
#endif
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    if (WriteVersionTxtToFile(app, fileName) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to dump version.txt in file -- %s\n", fileName);
    }

    if (RMH_StatusWriteToFile(app->rmh, fileName) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to dump the status summary in file -- %s\n", fileName);
    }

    if (RMH_Log_SetFilename(app->rmh, fileName) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set log filename to -- %s\n", fileName);
        return RMH_FAILURE;
    }

    RMH_PrintLog("MoCA now logging to -- %s\n", fileName);

    LOCAL_log(app);

    return RMH_SUCCESS;
}

int main(int argc, char *argv[])
{
    RMHApp appStr;
    RMHApp* app=&appStr;
    uint32_t i;
    RMH_Result result;

    memset(app, 0, sizeof(*app));

    /* No params provided then we will show a menu */
    app->interactive = (argc <= 1);
    app->argc=argc;
    app->argv=argv;

    /* Setup main list of APIs and add it to the list-of-lists */
    strcpy(app->allAPIs.name, "All APIs");
    app->subLists[app->numSubLists++]=&app->allAPIs;

    /* Connect to MoCA */
    app->rmh=RMH_Initialize(app->interactive ? EventCallback : NULL, (void*)&app);
    if (!app->rmh){
        RMH_PrintErr("Failed in RMH_Initialize!\n");
        return RMH_FAILURE;
    }

    /* Add RMH specific functions */
    ADD_API(LOCAL,                      LOCAL_log);
    ADD_API(LOCAL,                      LOCAL_log_forever);
    ADD_API(LOCAL,                      LOCAL_log_stop);

    /* Add local functions to the list */
    ADD_API(NO_PARAMS,                  RMH_Start);
    ADD_API(NO_PARAMS,                  RMH_Stop);
    ADD_API(NO_PARAMS,                  RMH_Status);
    ADD_API(IN_STRING,                  RMH_StatusWriteToFile);
    ADD_API(OUT_BOOL,                   RMH_Self_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_Self_SetEnabled);
    ADD_API(OUT_UINT32,                 RMH_Self_GetNodeId);
    ADD_API(OUT_UINT32,                 RMH_Self_GetLOF);
    ADD_API(IN_UINT32,                  RMH_Self_SetLOF);
    ADD_API(OUT_BOOL,                   RMH_Self_GetScanLOFOnly);
    ADD_API(IN_BOOL,                    RMH_Self_SetScanLOFOnly);
    ADD_API(OUT_MOCA_VERSION,           RMH_Self_GetHighestSupportedMoCAVersion);
    ADD_API(OUT_STRING,                 RMH_Self_GetSoftwareVersion);
    ADD_API(OUT_BOOL,                   RMH_Self_GetPreferredNCEnabled);
    ADD_API(IN_BOOL,                    RMH_Self_SetPreferredNCEnabled);
    ADD_API(OUT_UINT32,                 RMH_Self_GetPQOSEgressNumFlows);
    ADD_API(OUT_UINT32,                 RMH_Self_GetMaxPacketAggregation);
    ADD_API(IN_UINT32,                  RMH_Self_SetMaxPacketAggregation);
    ADD_API(OUT_UINT32_HEX,             RMH_Self_GetFreqMask);
    ADD_API(IN_UINT32_HEX,              RMH_Self_SetFreqMask);
    ADD_API(OUT_UINT32,                 RMH_Self_GetLowBandwidthLimit);
    ADD_API(IN_UINT32,                  RMH_Self_SetLowBandwidthLimit);
    ADD_API(OUT_UINT32,                 RMH_Self_GetMaxBitrate);
    ADD_API(OUT_UINT32,                 RMH_Self_GetTxBroadcastPhyRate);
    ADD_API(IN_INT32,                   RMH_Self_SetTxPowerLimit);
    ADD_API(OUT_INT32,                  RMH_Self_GetTxPowerLimit);
    ADD_API(OUT_UINT32_ARRAY,           RMH_Self_GetSupportedFrequencies);
    ADD_API(NO_PARAMS,                  RMH_Self_RestoreDefaultSettings);
    ADD_API(OUT_BOOL,                   RMH_Interface_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_Interface_SetEnabled);
    ADD_API(OUT_STRING,                 RMH_Interface_GetName);
    ADD_API(OUT_MAC,                    RMH_Interface_GetMac);
    ADD_API(IN_MAC,                     RMH_Interface_SetMac);
    ADD_API(OUT_UINT32,                 RMH_Network_GetLinkStatus);
    ADD_API(OUT_UINT32,                 RMH_Network_GetNumNodes);
    ADD_API(OUT_UINT32_NODELIST,        RMH_Network_GetNodeId);
    ADD_API(OUT_UINT32_NODELIST,        RMH_Network_GetAssociatedId);
    ADD_API(OUT_UINT32,                 RMH_Network_GetNCNodeId);
    ADD_API(OUT_MAC,                    RMH_Network_GetNCMac);
    ADD_API(OUT_UINT32,                 RMH_Network_GetBackupNCNodeId);
    ADD_API(OUT_MOCA_VERSION,           RMH_Network_GetMoCAVersion);
    ADD_API(OUT_BOOL,                   RMH_Network_GetMixedMode);
    ADD_API(OUT_UINT32,                 RMH_Network_GetLinkUptime);
    ADD_API(OUT_UINT32,                 RMH_Network_GetRFChannelFreq);
    ADD_API(OUT_UINT32,                 RMH_Network_GetPrimaryChannelFreq);
    ADD_API(OUT_UINT32,                 RMH_Network_GetSecondaryChannelFreq);
    ADD_API(OUT_UINT32,                 RMH_Network_GetTxMapPhyRate);
    ADD_API(OUT_UINT32,                 RMH_Network_GetTxGcdPowerReduction);
    ADD_API(OUT_UINT32_NODEMESH,        RMH_NetworkMesh_GetTxUnicastPhyRate);
    ADD_API(OUT_UINT32_NODEMESH,        RMH_NetworkMesh_GetBitLoadingInfo);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNode_GetNodeIdFromAssociatedId);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNode_GetAssociatedIdFromNodeId);
    ADD_API(IN_UINT32_OUT_MAC,          RMH_RemoteNode_GetMac);
    ADD_API(IN_UINT32_OUT_BOOL,         RMH_RemoteNode_GetPreferredNC);
    ADD_API(IN_UINT32_OUT_MOCA_VERSION, RMH_RemoteNode_GetHighestSupportedMoCAVersion);
    ADD_API(IN_UINT32_OUT_MOCA_VERSION, RMH_RemoteNode_GetActiveMoCAVersion);
    ADD_API(IN_UINT32_OUT_BOOL,         RMH_RemoteNode_GetQAM256Capable);
    ADD_API(IN_UINT32_OUT_BOOL,         RMH_RemoteNode_GetBondingCapable);
    ADD_API(IN_UINT32_OUT_BOOL,         RMH_RemoteNode_GetConcat);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNode_GetMaxPacketAggregation);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetPackets);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetUnicastPhyRate);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetBroadcastPhyRate);
    ADD_API(IN_UINT32_OUT_FLOAT,        RMH_RemoteNodeRx_GetUnicastPower);
    ADD_API(IN_UINT32_OUT_FLOAT,        RMH_RemoteNodeRx_GetBroadcastPower);
    ADD_API(IN_UINT32_OUT_FLOAT,        RMH_RemoteNodeRx_GetMapPower);
    ADD_API(IN_UINT32_OUT_FLOAT,        RMH_RemoteNodeRx_GetSNR);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetTotalErrors);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetCorrectedErrors);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeRx_GetUnCorrectedErrors);
    ADD_API(IN_UINT32_OUT_FLOAT,        RMH_RemoteNodeTx_GetUnicastPower);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeTx_GetPowerReduction);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeTx_GetUnicastPhyRate);
    ADD_API(IN_UINT32_OUT_UINT32,       RMH_RemoteNodeTx_GetPackets);
    ADD_API(OUT_BOOL,                   RMH_QAM256_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_QAM256_SetEnabled);
    ADD_API(OUT_UINT32,                 RMH_QAM256_GetTargetPhyRate);
    ADD_API(IN_UINT32,                  RMH_QAM256_SetTargetPhyRate);
    ADD_API(OUT_BOOL,                   RMH_Turbo_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_Turbo_SetEnabled);
    ADD_API(OUT_BOOL,                   RMH_Bonding_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_Bonding_SetEnabled);
    ADD_API(OUT_BOOL,                   RMH_Privacy_GetEnabled);
    ADD_API(IN_BOOL,                    RMH_Privacy_SetEnabled);
    ADD_API(OUT_STRING,                 RMH_Privacy_GetPassword);
    ADD_API(IN_STRING,                  RMH_Privacy_SetPassword);
    ADD_API(OUT_POWER_MODE,             RMH_Power_GetMode);
    ADD_API(OUT_UINT32_HEX,             RMH_Power_GetSupportedModes);
    ADD_API(OUT_BOOL,                   RMH_Power_GetTxPowerControlEnabled);
    ADD_API(IN_BOOL,                    RMH_Power_SetTxPowerControlEnabled);
    ADD_API(OUT_BOOL,                   RMH_Power_GetTxBeaconPowerReductionEnabled);
    ADD_API(IN_BOOL,                    RMH_Power_SetTxBeaconPowerReductionEnabled);
    ADD_API(OUT_UINT32,                 RMH_Power_GetTxBeaconPowerReduction);
    ADD_API(IN_UINT32,                  RMH_Power_SetTxBeaconPowerReduction);
    ADD_API(OUT_UINT32,                 RMH_Taboo_GetStartChannel);
    ADD_API(IN_UINT32,                  RMH_Taboo_SetStartChannel);
    ADD_API(OUT_UINT32_HEX,             RMH_Taboo_GetChannelMask);
    ADD_API(IN_UINT32_HEX,              RMH_Taboo_SetChannelMask);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxTotalBytes);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxTotalBytes);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxTotalPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxTotalPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxUnicastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxUnicastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxBroadcastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxBroadcastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxMulticastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxMulticastPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxReservationRequestPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxReservationRequestPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxMapPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxMapPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxLinkControlPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxLinkControlPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxUnknownProtocolPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxDroppedPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxDroppedPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxTotalErrors);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxTotalErrors);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxCRCErrors);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxTimeoutErrors);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetAdmissionAttempts);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetAdmissionSucceeded);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetAdmissionFailures);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetAdmissionsDeniedAsNC);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxBeacons);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxBeacons);
    ADD_API(OUT_UINT32_NODELIST,        RMH_Stats_GetRxCorrectedErrors);
    ADD_API(OUT_UINT32_NODELIST,        RMH_Stats_GetRxUncorrectedErrors);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetRxTotalAggregatedPackets);
    ADD_API(OUT_UINT32,                 RMH_Stats_GetTxTotalAggregatedPackets);
    ADD_API(OUT_UINT32_ARRAY,           RMH_Stats_GetRxPacketAggregation);
    ADD_API(OUT_UINT32_ARRAY,           RMH_Stats_GetTxPacketAggregation);
    ADD_API(NO_PARAMS,                  RMH_Stats_Reset);
    ADD_API(OUT_LOGLEVEL,               RMH_Log_GetLevel);
    ADD_API(IN_LOGLEVEL,                RMH_Log_SetLevel);
    ADD_API(OUT_STRING,                 RMH_Log_GetFilename);
    ADD_API(IN_STRING,                  RMH_Log_SetFilename);
    ADD_API(IN_RMH_ENUM,                RMH_ResultToString);

    /* Setup complete, execute program */
    result=app->interactive ? DoInteractive(app) : DoNonInteractive(app);

    /* Cleanup */
    for (i=0; i != app->allAPIs.apiListSize; i++) {
        free(app->allAPIs.apiList[i]);
    }
    RMH_Destroy(app->rmh);

    return result;
}
