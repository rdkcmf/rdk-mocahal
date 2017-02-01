#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
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

static
RMH_Result ReadLine(const char *prompt, const RMHApp *app, char *buf, const uint32_t bufSize) {
    char *input=NULL;
    if (app->interactive) {
        if (prompt)
            printf("%s", prompt);
        input=fgets(buf, bufSize, stdin);
        if (input) {
            input[strcspn(input, "\n")] = '\0';
        }
    }
    else {
        input=(app->argc >= 1) ? app->argv[0] : NULL;
        if (input) {
            strncpy(buf, input, bufSize);
            buf[bufSize]='\0';
        }
    }

    return (input && strlen(input)) ? RMH_SUCCESS : RMH_FAILURE;
}

static
RMH_Result ReadUint32(const RMHApp *app, uint32_t *value) {
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
RMH_Result ReadUint32Hex(const RMHApp *app, uint32_t *value) {
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
RMH_Result ReadBool(const RMHApp *app, bool *value) {
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
RMH_Result ReadString(const RMHApp *app, char *buf, const uint32_t bufSize) {
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
    printf("\n\n");
    switch(event) {
    case LINK_STATUS_CHANGED:
        printf("%p: Link status changed to %s\n", userContext, RMH_LinkStatusString(eventData->LINK_STATUS_CHANGED.status));
        break;
    case MOCA_VERSION_CHANGED:
        printf("%p: MoCA Version Changed to %s\n", userContext, RMH_MoCAVersionToString(eventData->MOCA_VERSION_CHANGED.version));
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
FindAPI(const char *apiName, const RMHApp *app) {
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
FindAPIList(const char *listName, const RMHApp *app) {
    uint32_t i;
    for (i=0; i != app->numSubLists; i++) {
        if(strncmp(listName, app->subLists[i]->name,  strlen(app->subLists[i]->name)) == 0) {
            return app->subLists[i];
        }
    }

  return NULL;
}


void DisplayList(const RMHApp *app, const RMHApp_APIList *activeList) {
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

void DoInteractive(const RMHApp *app) {
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
RMH_Result GET_UINT32(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%u\n", response);
    }
    return ret;
}

static
RMH_Result GET_UINT32_HEX(const RMHApp *app, RMH_Result (*api)(RMH_Handle handle, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("0x%08x\n", response);
    }
    return ret;
}


static
RMH_Result GET_BOOL(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, bool* response)) {
    bool response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response ? "TRUE" : "FALSE");
    }
    return ret;
}

static
RMH_Result GET_STRING(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    RMH_Result ret = api(app->rmh, response, sizeof(response));
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response);
    }
    return ret;
}

static
RMH_Result GET_NODE_STRING(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, uint8_t nodeId, char* responseBuf, const size_t responseBufSize)) {
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
RMH_Result GET_POWER_MODE(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_PowerMode* response)) {
    RMH_PowerMode response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", RMH_PowerModeToString(response));
    }
    return ret;
}

static
RMH_Result GET_MOCA_VERSION(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", RMH_MoCAVersionToString(response));
    }
    return ret;
}

static
RMH_Result GET_NET_STATUS(const RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkStatus* response)) {
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
RMH_Result GET_UINT32_NETLIST(const RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkList_Uint32_t* response)) {
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
RMH_Result GET_UINT32_NETMESH(const RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkMesh_Uint32_t* response)) {
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
RMH_Result SET_BOOL(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const bool value)) {
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
RMH_Result SET_UINT32(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
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
RMH_Result SET_UINT32_HEX(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const uint32_t value)) {
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
RMH_Result SET_STRING(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const char* value)) {
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
RMH_Result NO_PARAMS(const RMHApp *app, RMH_Result (*api)(RMH_Handle rmh)) {
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

    memset(&app, 0, sizeof(app));
    strcpy(app.allAPIs.name, "All APIs");
    app.subLists[app.numSubLists++]=&app.allAPIs;
    app.interactive = (argc <= 1);

    app.libHandle = dlopen("/usr/lib/librdkmocahal.so.0.0.0", RTLD_LOCAL | RTLD_LAZY);
    if (!app.libHandle){
        printf("Unable to open library /usr/lib/librdkmocahal.so.0.0.0!\n");
        return 1;
    }

    app.rmh=RMH_Initialize();
    if (!app.libHandle){
        printf("Failed in RMH_Initialize!\n");
        return 1;
    }

    RMH_Callback_RegisterEvent(app.rmh, EventCallback, (void*)&app, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED);

    ADD_API(GET_UINT32,         RMH_Self_GetNodeId);
    ADD_API(GET_UINT32,         RMH_Self_GetLOF);
    ADD_API(SET_UINT32,         RMH_Self_SetLOF);
    ADD_API(GET_BOOL,           RMH_Self_GetScanLOFOnly);
    ADD_API(SET_BOOL,           RMH_Self_SetScanLOFOnly);
    ADD_API(GET_MOCA_VERSION,   RMH_Self_GetHighestSupportedMoCAVersion);
    ADD_API(GET_STRING,         RMH_Self_GetMacString);
    ADD_API(GET_STRING,         RMH_Self_GetSoftwareVersion);
    ADD_API(GET_BOOL,           RMH_Self_GetPreferredNCEnabled);
    ADD_API(SET_BOOL,           RMH_Self_SetPreferredNCEnabled);
    ADD_API(GET_BOOL,           RMH_Self_GetInterfaceEnabled);
    ADD_API(SET_BOOL,           RMH_Self_SetInterfaceEnabled);
    ADD_API(GET_UINT32,         RMH_Self_GetPQOSEgressNumFlows);
    ADD_API(GET_UINT32,         RMH_Self_GetMaxPacketAggr);
    ADD_API(SET_UINT32,         RMH_Self_SetMaxPacketAggr);

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

    ADD_API(GET_MOCA_VERSION,   RMH_Network_GetMoCAVersion);
    ADD_API(GET_UINT32,         RMH_Network_GetLinkUptime);
    ADD_API(GET_UINT32,         RMH_Network_GetRFChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetPrimaryChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetSecondaryChannelFreq);
    ADD_API(GET_UINT32,         RMH_Network_GetLinkStatus);
    ADD_API(GET_UINT32,         RMH_Network_GetNumNodes);
    ADD_API(GET_UINT32,         RMH_Network_GetNCNodeId);
    ADD_API(GET_STRING,         RMH_Network_GetNCMacString);
    ADD_API(GET_UINT32,         RMH_Network_GetBackupNCNodeId);
    ADD_API(GET_NODE_STRING,    RMH_Network_GetNodeMacString);
    ADD_API(GET_NET_STATUS,     RMH_Network_GetStatusTx);
    ADD_API(GET_NET_STATUS,     RMH_Network_GetStatusRx);
    ADD_API(GET_UINT32_NETMESH, RMH_Network_GetBitLoadingInfo);

    ADD_API(GET_UINT32,         RMH_Power_GetBeaconPowerReduction);
    ADD_API(SET_UINT32,         RMH_Power_SetBeaconPowerReduction);
    ADD_API(GET_BOOL,           RMH_Power_GetBeaconPowerReductionEnabled);
    ADD_API(SET_BOOL,           RMH_Power_SetBeaconPowerReductionEnabled);
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

    if (app.interactive) {
       DoInteractive(&app);
    }
    else {
        RMHApp_API *api=FindAPI(argv[1], &app);
        if (api) {
            /* We've found our API. Pass along argc and argv skipping the program name and API */
            app.argc=argc-2;
            app.argv=&argv[2];
            if (!api->apiFunc) {
                printf("That API has not been implemented by the HAL library.\n");
                return RMH_UNIMPLEMENTED;
            }
            else {
                return api->apiHandlerFunc(&app, api->apiFunc);
            }
        }
        else {
            printf("The API '%s' is invalid! Please provide a valid command.\n", argv[1]);
            return 1;
        }
    }

    for (i=0; i != app.allAPIs.apiListSize; i++) {
        free(app.allAPIs.apiList[i]);
    }

    RMH_Destroy(app.rmh);
    return 0;
}
