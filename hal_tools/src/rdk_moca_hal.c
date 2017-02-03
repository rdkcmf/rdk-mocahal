#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "rdk_moca_hal.h"

typedef struct RMHApp_APIList {
    char name[32];
    struct RMHApp_API *apiList[2048];
    uint32_t apiListSize;
} RMHApp_APIList;

typedef struct RMHApp {
    RMH_Handle rmh;
    void *libHandle;
    bool interactive;
    int argc;
    char **argv;
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
    RMHApp_AddAPI(&app, #_api, _handler, _api); \
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
RMH_Result ReadLine(const char *prompt, RMHApp *app, char *buf, const uint32_t bufSize) {
    const char *input=NULL;
    if (app->interactive) {
        if (prompt)
            printf("%s", prompt);
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

    return (input && strlen(input)) ? RMH_SUCCESS : RMH_FAILURE;
}

static
RMH_Result ReadUint32(RMHApp *app, uint32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (number): ", app, input, sizeof(input)) == RMH_SUCCESS) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 10);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    printf("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadUint32Hex(RMHApp *app, uint32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (Hex): ", app, input, sizeof(input)) == RMH_SUCCESS) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 0);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    printf("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadBool(RMHApp *app, bool *value) {
    char input[16];
    if (ReadLine("Enter your choice (TRUE,FALSE or 1,0): ", app, input, sizeof(input)) == RMH_SUCCESS) {
        if ((strcmp(input, "1") == 0) || (strcasecmp(input, "TRUE") == 0)) {
            *value=1;
            return RMH_SUCCESS;
        }
        else if ((strcmp(input, "0") == 0) || (strcasecmp(input, "FALSE") == 0)) {
            *value=0;
            return RMH_SUCCESS;
        }
    }
    printf("Bad input. Please use only TRUE,FALSE or 1,0\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadString(RMHApp *app, char *buf, const uint32_t bufSize) {
    if (ReadLine("Enter your choice: ", app, buf, bufSize) == RMH_SUCCESS) {
        return RMH_SUCCESS;
    }
    printf("Bad input. Please provide a valid string.\n");
    return RMH_FAILURE;
}





/***********************************************************
 * Callbacks
 ***********************************************************/

static void EventCallback(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext){
    switch(event) {
    case LINK_STATUS_CHANGED:
        printf("\n%p: Link status changed to %s\n", userContext, RMH_LinkStatusToString(eventData->LINK_STATUS_CHANGED.status));
        break;
    case MOCA_VERSION_CHANGED:
        printf("\n%p: MoCA Version Changed to %s\n", userContext, RMH_MoCAVersionToString(eventData->MOCA_VERSION_CHANGED.version));
        break;
    case RMH_LOG_PRINT:
       {
        struct tm* tm_info;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        tm_info = localtime(&tv.tv_sec);
        printf("%02d.%02d.%02d %02d:%02d:%02d:%03d %s", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec/1000, eventData->RMH_LOG_PRINT.logMsg);
        }

        break;
    default:
        printf("%p: Unhandled MoCA event %u!\n", event);
        break;
    }
}




/***********************************************************
 * Generic Helper Functions
 ***********************************************************/

static RMHApp_API *
FindAPI(const char *apiName, RMHApp *app) {
    uint32_t i;
    uint32_t offset=0;
    if(strncmp(apiName, "RMH_", 4) != 0) {
        /* The user did not provide "RMH_" as part of the API name. Skip this when checking the list */
        offset = 4;
    }

    for (i=0; i != app->allAPIs.apiListSize; i++) {
        if(strncmp(apiName, app->allAPIs.apiList[i]->name+offset,  strlen(app->allAPIs.apiList[i]->name)-offset) == 0) {
            return app->allAPIs.apiList[i];
        }
    }

  return NULL;
}

static RMHApp_APIList *
FindAPIList(const char *listName, RMHApp *app) {
    uint32_t i;
    for (i=0; i != app->numSubLists; i++) {
        if(strncmp(listName, app->subLists[i]->name,  strlen(app->subLists[i]->name)) == 0) {
            return app->subLists[i];
        }
    }

  return NULL;
}


void DisplayList(RMHApp *app, const RMHApp_APIList *activeList) {
    uint32_t i;
    printf("\n\n");
    if (activeList) {
        printf("%02d. %s\n", 1, "Go Back");
        for (i=0; i != activeList->apiListSize; i++) {
            if (activeList->apiList[i]->apiFunc) {
                printf("%02d. %s\n", i+2, activeList->apiList[i]->name);
            }
            else {
                printf("%02d. %s [UNIMPLEMENTED]\n", i+2, activeList->apiList[i]->name);
            }
        }
    }
    else{
        printf("%02d. %s\n", 1, "Exit");
        for (i=0; i != app->numSubLists; i++) {
            printf("%02d. %s\n", i+2, app->subLists[i]->name);
        }
    }
}

RMH_Result DoInteractive(RMHApp *app) {
    char inputBuf[8];
    char *input;
    uint32_t option;
    RMHApp_APIList *activeList=NULL;

    DisplayList(app, activeList);
    while(true) {
        if (ReadUint32(app, &option)==RMH_SUCCESS) {
            if (option != 0) {
                if ( option == 1 ) {
                    if (!activeList) return;
                    activeList=NULL;
                }
                else {
                    option-=2;
                    if (activeList && option<activeList->apiListSize) {
                        RMH_Result ret = RMH_UNIMPLEMENTED;
                        printf("----------------------------------------------------------------------------\n");
                        printf("%s:\n", activeList->apiList[option]->name);
                        if (activeList->apiList[option]->apiFunc) {
                            ret=activeList->apiList[option]->apiHandlerFunc(app, activeList->apiList[option]->apiFunc);
                        }
                        if (ret != RMH_SUCCESS) {
                            printf("**Error %s!\n", RMH_ResultToString(ret));
                        }
                        printf("----------------------------------------------------------------------------\n\n\n");
                        continue;
                    }
                    else if (!activeList && option < app->numSubLists ) {
                        activeList=app->subLists[option];
                        DisplayList(app, activeList);
                        continue;
                    }
                }
            }
            printf("**Error: Invalid selection\n");
        }
        DisplayList(app, activeList);
    }
    return RMH_SUCCESS;
}

RMH_Result DoNonInteractive(RMHApp *app) {
    const char* option;
    bool monitorCallbacks=false;

    /* First read to drop program name */
    ReadNextArg(app);

    do {
        option=ReadNextArg(app);
        if (!option) break;

        /* This is an option, not an API */
        if (strlen(option) == 2){
            switch (option[1]) {
            case 'm': monitorCallbacks = true; break;
            default:
                printf("The option '%c'' not supported!\n", option[1]);
                return RMH_UNIMPLEMENTED;
                break;
            }
        }
        else {
            /* This isn't an option, it must be an API call */
            RMHApp_API *api=FindAPI(option, app);
            if (api && api->apiFunc) {
                RMH_Result ret=api->apiHandlerFunc(app, api->apiFunc);
                if (ret != RMH_SUCCESS) {
                    return ret;
                }
            }
            else {
                printf("That API '%s'' is either not implemented or does not exist!\n", option);
                return RMH_UNIMPLEMENTED;
            }
        }
    } while(option);

    if (monitorCallbacks) {
        printf("Monitoring for callbacks and MoCA logs. Press enter to exit...\n");
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
    *(void **)(&newItem->apiFunc) = dlsym(app->libHandle, apiName);
    app->allAPIs.apiList[app->allAPIs.apiListSize++]=newItem;

    if ((strncmp(apiName,"RMH_",4) == 0) && ((lastChar = strchr(&apiName[4], '_')) != NULL)) {
        uint32_t copySize= (lastChar-&apiName[4]) > sizeof(subListName) ? sizeof(subListName) : (lastChar-&apiName[4]);
        strncpy(subListName, &apiName[4], copySize);
        subListName[copySize]='\0';
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
 * API Handler Functions
 ***********************************************************/
static
RMH_Result GET_UINT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%u\n", response);
    }
    return ret;
}

static
RMH_Result GET_UINT32_HEX(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("0x%08x\n", response);
    }
    return ret;
}


static
RMH_Result GET_BOOL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, bool* response)) {
    bool response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response ? "TRUE" : "FALSE");
    }
    return ret;
}

static
RMH_Result GET_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    RMH_Result ret = api(app->rmh, response, sizeof(response));
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response);
    }
    return ret;
}

static
RMH_Result GET_NODE_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, uint8_t nodeId, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    uint32_t nodeId;
    RMH_Result ret=ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        RMH_Result ret = api(app->rmh, nodeId, response, sizeof(response));
        if (ret == RMH_SUCCESS) {
            printf("%s\n", response);
        }
    }
    return ret;
}

static
RMH_Result GET_POWER_MODE(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_PowerMode* response)) {
    RMH_PowerMode response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", RMH_PowerModeToString(response));
    }
    return ret;
}

static
RMH_Result GET_MOCA_VERSION(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", RMH_MoCAVersionToString(response));
    }
    return ret;
}

static
RMH_Result GET_LOGLEVEL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_LogLevel* response)) {
    RMH_LogLevel response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", RMH_LogLevelToString(response));
    }
    return ret;
}

static
RMH_Result GET_NET_STATUS(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkStatus* response)) {
    RMH_NetworkStatus response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i;

        printf("Self node:\n");
        printf("  nodeId:%u\n", response.localNodeId);
        printf("  neighNodeId:%u\n", response.localNeighNodeId);
        printf("\n");

        printf("NC node:\n");
        printf("  nodeId:%u\n", response.ncNodeId);
        printf("  neighNodeId:%u\n", response.ncNeighNodeId);
        printf("\n");

        printf("Remote nodes:\n");
        for (i = 0; i < response.remoteNodesPresent; i++) {
            printf("  nodeId:%u\n", response.remoteNodes[i].nodeId);
            printf("  neighNodeId:%u\n", response.remoteNodes[i].neighNodeId);
            printf("  phyRate:%u\n", response.remoteNodes[i].phyRate);
            printf("  power:%d\n", response.remoteNodes[i].power);
            printf("  backoff:%u\n", response.remoteNodes[i].backoff);
            printf("\n");
        }
    }
    return ret;
}


static
RMH_Result GET_UINT32_NETLIST(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkList_Uint32_t* response)) {
    RMH_NetworkList_Uint32_t response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i;
        for (i = 0; i < response.numNodes; i++) {
            printf("NodeId:%u -- %u\n", response.nodeId[i], response.nodeValue[i]);
        }
    }
    return ret;
}

static
RMH_Result GET_UINT32_NETMESH(RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkMesh_Uint32_t* response)) {
    RMH_NetworkMesh_Uint32_t response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i, j;

        for (i = 0; i < response.numNodes; i++) {
            printf("NodeId:%u\n", response.nodeId[i]);
            RMH_NetworkList_Uint32_t *nl = &response.nodeValue[i];
            for (j = 0; j < nl->numNodes; j++) {
                printf("   %u: %u\n", nl->nodeId[j], nl->nodeValue[j]);
            }
            printf("\n");
        }
    }
    return ret;
}

static
RMH_Result SET_BOOL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const bool value)) {
    bool value;
    RMH_Result ret=ReadBool(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %s\n", value ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result SET_UINT32(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=ReadUint32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result SET_UINT32_HEX(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=ReadUint32Hex(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to 0x%08x\n", value);
        }
    }
    return ret;
}

static
RMH_Result SET_STRING(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const char* value)) {
    uint32_t value;
    char input[256];
    RMH_Result ret=ReadString(app, input, sizeof(input));
    if (ret == RMH_SUCCESS) {
        printf("Setting to %s\b", input);
        ret = api(app->rmh, input);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result SET_LOGLEVEL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const RMH_LogLevel response)) {
    RMH_LogLevel value;
    int i;

    for (i=0; app->interactive && i != sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0]); i++ ) {
        printf("%d. %s\n", i, RMH_LogLevelStr[i]);
    }
    RMH_Result ret=ReadUint32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result NO_PARAMS(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh)) {
    RMH_Result ret = api(app->rmh);
    if (ret == RMH_SUCCESS) {
        printf("Success\n");
    }
    return ret;
}


int main(int argc, char *argv[])
{
    RMHApp app;
    uint32_t i;
    RMH_Result result;

    memset(&app, 0, sizeof(app));

    /* No params provided then we will show a menu */
    app.interactive = (argc <= 1);
    app.argc=argc;
    app.argv=argv;

    /* Setup main list of APIs and add it to the list-of-lists */
    strcpy(app.allAPIs.name, "All APIs");
    app.subLists[app.numSubLists++]=&app.allAPIs;

    /* Manually local HAL libarary so we can dynamically check for each API to exist. This ensures that adding new APIs to
       the HAL won't require every HAL implemention to immediately change */
    app.libHandle = dlopen("/usr/lib/librdkmocahal.so.0.0.0", RTLD_LOCAL | RTLD_LAZY);
    if (!app.libHandle){
        printf("Unable to open library /usr/lib/librdkmocahal.so.0.0.0!\n");
        return 1;
    }

    /* Connect to MoCA */
    app.rmh=RMH_Initialize();
    if (!app.libHandle){
        printf("Failed in RMH_Initialize!\n");
        return 1;
    }

    /* Setup callback notifications */
    RMH_Callback_RegisterEvent(app.rmh, EventCallback, (void*)&app, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED | RMH_LOG_PRINT);

    /* Add APIs to the list */
    ADD_API(GET_UINT32,         RMH_Self_GetNodeId);
    ADD_API(GET_UINT32,         RMH_Self_GetLOF);
    ADD_API(SET_UINT32,         RMH_Self_SetLOF);
    ADD_API(GET_BOOL,           RMH_Self_GetScanLOFOnly);
    ADD_API(SET_BOOL,           RMH_Self_SetScanLOFOnly);
    ADD_API(GET_MOCA_VERSION,   RMH_Self_GetHighestSupportedMoCAVersion);
    ADD_API(GET_STRING,         RMH_Self_GetSoftwareVersion);
    ADD_API(GET_BOOL,           RMH_Self_GetPreferredNCEnabled);
    ADD_API(SET_BOOL,           RMH_Self_SetPreferredNCEnabled);
    ADD_API(GET_UINT32,         RMH_Self_GetPQOSEgressNumFlows);
    ADD_API(GET_UINT32,         RMH_Self_GetMaxPacketAggr);
    ADD_API(SET_UINT32,         RMH_Self_SetMaxPacketAggr);
    ADD_API(GET_UINT32_HEX,     RMH_Self_GetFreqMask);
    ADD_API(SET_UINT32_HEX,     RMH_Self_SetFreqMask);

    ADD_API(GET_BOOL,           RMH_Interface_GetEnabled);
    ADD_API(SET_BOOL,           RMH_Interface_SetEnabled);
    ADD_API(GET_STRING,         RMH_Interface_GetName);
    ADD_API(GET_STRING,         RMH_Interface_GetMacString);

    ADD_API(GET_BOOL,           RMH_QAM256_GetCapable);
    ADD_API(SET_BOOL,           RMH_QAM256_SetCapable);
    ADD_API(GET_UINT32,         RMH_QAM256_GetTargetPhyRate);
    ADD_API(SET_UINT32,         RMH_QAM256_SetTargetPhyRate);

    ADD_API(GET_BOOL,           RMH_Turbo_GetEnabled);
    ADD_API(SET_BOOL,           RMH_Turbo_SetEnabled);

    ADD_API(GET_STRING,         RMH_Privacy_GetPassword);
    ADD_API(SET_STRING,         RMH_Privacy_SetPassword);
    ADD_API(GET_BOOL,           RMH_Privacy_GetEnabled);
    ADD_API(SET_BOOL,           RMH_Privacy_SetEnabled);

    ADD_API(GET_POWER_MODE,     RMH_Power_GetMode);
    ADD_API(GET_UINT32_HEX,     RMH_Power_GetSupportedModes);

    ADD_API(GET_UINT32,         RMH_Network_GetLinkStatus);
    ADD_API(GET_MOCA_VERSION,   RMH_Network_GetMoCAVersion);
    ADD_API(GET_UINT32,         RMH_Network_GetLinkUptime);
    ADD_API(GET_UINT32,         RMH_Network_GetRFChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetPrimaryChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetSecondaryChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetNumNodes);
    ADD_API(GET_UINT32,         RMH_Network_GetNCNodeId);
    ADD_API(GET_STRING,         RMH_Network_GetNCMacString);
    ADD_API(GET_UINT32,         RMH_Network_GetBackupNCNodeId);
    ADD_API(GET_NODE_STRING,    RMH_Network_GetNodeMacString);
    ADD_API(GET_NET_STATUS,     RMH_Network_GetStatusTx);
    ADD_API(GET_NET_STATUS,     RMH_Network_GetStatusRx);
    ADD_API(GET_UINT32_NETMESH, RMH_Network_GetBitLoadingInfo);

    ADD_API(GET_UINT32,         RMH_Power_GetTxGcdPowerReduction);
    ADD_API(GET_UINT32_NETLIST, RMH_Power_GetTxGcdPowerReductionList);
    ADD_API(GET_BOOL,           RMH_Power_GetBeaconPowerReductionEnabled);
    ADD_API(SET_BOOL,           RMH_Power_SetBeaconPowerReductionEnabled);
    ADD_API(GET_UINT32,         RMH_Power_GetBeaconPowerReduction);
    ADD_API(SET_UINT32,         RMH_Power_SetBeaconPowerReduction);
    ADD_API(GET_UINT32,         RMH_Power_GetTPCEnabled);
    ADD_API(SET_UINT32,         RMH_Power_SetTPCEnabled);

    ADD_API(GET_UINT32,         RMH_Stats_GetFrameCRCErrors);
    ADD_API(GET_UINT32,         RMH_Stats_GetFrameTimeoutErrors);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionAttempts);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionSucceeded);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionFailures);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionsDeniedAsNC);
    ADD_API(GET_UINT32,         RMH_Stats_GetTxTotalPackets);
    ADD_API(GET_UINT32,         RMH_Stats_GetTxDroppedPackets);
    ADD_API(GET_UINT32,         RMH_Stats_GetRxTotalPackets);
    ADD_API(GET_UINT32,         RMH_Stats_GetRxDroppedPackets);
    ADD_API(GET_UINT32_NETLIST, RMH_Stats_GetRxTotalCorrectedCodeWordErrors);
    ADD_API(GET_UINT32_NETLIST, RMH_Stats_GetRxTotalUncorrectedCodeWordErrors);
    ADD_API(NO_PARAMS,          RMH_Stats_Reset);

    ADD_API(GET_UINT32,         RMH_Taboo_GetStartRFChannel);
    ADD_API(SET_UINT32,         RMH_Taboo_SetStartRFChannel);
    ADD_API(GET_UINT32_HEX,     RMH_Taboo_GetFreqMask);
    ADD_API(SET_UINT32_HEX,     RMH_Taboo_SetFreqMask);

    ADD_API(GET_LOGLEVEL,       RMH_Log_GetLevel);
    ADD_API(SET_LOGLEVEL,       RMH_Log_SetLevel);

    /* Setup complete, execute program */
    result=app.interactive ? DoInteractive(&app) : DoNonInteractive(&app);

    /* Cleanup */
    for (i=0; i != app.allAPIs.apiListSize; i++) {
        free(app.allAPIs.apiList[i]);
    }
    RMH_Destroy(app.rmh);

    return result;
}
