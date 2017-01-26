#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rdk_moca_hal.h"


typedef struct RMHApp {
    RMH_Handle rmh;
    bool interactive;
    int argc;
    char **argv;

    struct RMHApp_API *apiList[2048];
    uint32_t apiListSize;
} RMHApp;

typedef struct RMHApp_API {
    const char *name;
    RMH_Result (*apiHandlerFunc)(const struct RMHApp *app, void *api);
    void *apiFunc;
} RMHApp_API;


/* This macro first verifies the API matches what the handler expects. This is important because we store both the api
   and handler as (void*) so this line makes sure we catch any type issues at compile time */
#define ADD_API(_handler, _api) { \
    while(0) _handler(NULL, _api); \
    RMHApp_AddAPI(&app, #_api, _handler, _api); \
}

static inline
void RMHApp_AddAPI(RMHApp* app, const char* apiName, void *apiHandlerFunc, void *apiFunc) {
    RMHApp_API *newItem=(RMHApp_API *)malloc(sizeof(RMHApp_API));
    memset(newItem, 0, sizeof(RMHApp_API));
    newItem->apiHandlerFunc=(void*)apiHandlerFunc;
    newItem->name=apiName;
    newItem->apiFunc=apiFunc;
    app->apiList[app->apiListSize++]=newItem;
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
        printf("Got input %s\n", input);
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
    switch(event) {
    case LINK_STATUS_CHANGED:
        printf("%p: Link status changed to %u\n", userContext, eventData->LINK_STATUS_CHANGED.status);
        break;
    case MOCA_VERSION_CHANGED:
        printf("%p: MoCA Version Changed to %s\n", userContext, eventData->MOCA_VERSION_CHANGED.version);
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

    for (i=0; i != app->apiListSize; i++) {
        if(strncmp(apiName, app->apiList[i]->name+offset,  strlen(app->apiList[i]->name)-offset) == 0) {
            return app->apiList[i];
        }
    }

  return NULL;
}


void DisplayMenu(const RMHApp *app) {
    uint32_t i;
    printf("\n\n");
    for (i=0; i != app->apiListSize; i++) {
        printf("%02d. %s\n", i+1, app->apiList[i]->name);
    }
    printf("%02d. %s\n", i+1, "Exit");
}

void DoInteractive(const RMHApp *app) {
    char inputBuf[8];
    char *input;
    uint32_t option;

    DisplayMenu(app);
    while(true) {
        if (ReadUint32(app, &option)==RMH_SUCCESS) {
            if (option != 0) {
                if ( option <= app->apiListSize ) {
                    option--;
                    RMH_Result ret;
                    printf("----------------------------------------------------------------------------\n");
                    printf("%s:\n", app->apiList[option]->name);
                    ret=app->apiList[option]->apiHandlerFunc(app, app->apiList[option]->apiFunc);
                    if (ret != RMH_SUCCESS) {
                        printf("**Error %s!\n", RMH_ResultToString(ret));
                    }
                    printf("----------------------------------------------------------------------------\n\n\n");
                    continue;
                }
                else if ( option == app->apiListSize+1 ) {
                    printf("Exiting...\n");
                    return;
                }
            }
            printf("**Error: Invalid selection\n");
        }
        DisplayMenu(app);
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
RMH_Result GET_BIT_LOADING(const RMHApp *app, RMH_Result (*api)(RMH_Handle handle, RMH_NetworkBitLoadingInfo* response)) {
    RMH_NetworkBitLoadingInfo response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i, j;

        for (i = 0; i < response.remoteNodesPresent; i++) {
            printf("NodeId:%u\n", response.remoteNodes[i].nodeId);
            RMH_NodeBitLoadingInfo *nodeBitLoading = &response.remoteNodes[i];
            for (j = 0; j < response.remoteNodesPresent; j++) {
                printf("   %u: %u\n", nodeBitLoading->connectedNodes[j].nodeId, nodeBitLoading->connectedNodes[j].bitloading);
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
    app.interactive = (argc <= 1);
    app.rmh=RMH_Initialize();
    RMH_Callback_RegisterEvent(app.rmh, EventCallback, (void*)&app, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED);

    ADD_API(GET_UINT32,         RMH_Self_GetNodeId);
    ADD_API(GET_UINT32,         RMH_Self_GetLOF);
    ADD_API(GET_STRING,         RMH_Self_GetHighestSupportedMoCAVersion);
    ADD_API(GET_STRING,         RMH_Self_GetMacString);
    ADD_API(GET_STRING,         RMH_Self_GetSoftwareVersion);
    ADD_API(GET_BOOL,           RMH_Self_GetPreferredNCEnabled);
    ADD_API(SET_BOOL,           RMH_Self_SetPreferredNCEnabled);

    ADD_API(GET_BOOL,           RMH_QAM256_GetCapable);
    ADD_API(SET_BOOL,           RMH_QAM256_SetCapable);

    ADD_API(GET_BOOL,           RMH_Turbo_GetEnabled);
    ADD_API(SET_BOOL,           RMH_Turbo_SetEnabled);

    ADD_API(GET_STRING,         RMH_Privacy_GetPassword);
    ADD_API(SET_STRING,         RMH_Privacy_SetPassword);
    ADD_API(GET_BOOL,           RMH_Privacy_GetEnabled);
    ADD_API(SET_BOOL,           RMH_Privacy_SetEnabled);

    ADD_API(GET_POWER_MODE,     RMH_Power_GetMode);
    ADD_API(GET_UINT32_HEX,     RMH_Power_GetSupportedModes);

    ADD_API(GET_STRING,         RMH_Network_GetMoCAVersion);
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
    ADD_API(GET_BIT_LOADING,    RMH_Network_GetBitLoadingInfo);

    ADD_API(GET_UINT32,         RMH_Power_GetBeaconPowerReduction);
    ADD_API(GET_BOOL,           RMH_Power_GetBeaconPowerReductionEnabled);
    ADD_API(SET_BOOL,           RMH_Power_SetBeaconPowerReductionEnabled);
    ADD_API(SET_UINT32,         RMH_Power_SetBeaconPowerReduction);

    ADD_API(GET_UINT32,         RMH_Stats_GetFrameCRCErrors);
    ADD_API(GET_UINT32,         RMH_Stats_GetFrameTimeoutErrors);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionAttempts);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionSucceeded);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionFailures);
    ADD_API(GET_UINT32,         RMH_Stats_GetAdmissionsDeniedAsNC);
    ADD_API(NO_PARAMS,          RMH_Stats_Reset);


    if (app.interactive) {
       DoInteractive(&app);
    }
    else {
        RMHApp_API *api=FindAPI(argv[1], &app);
        if (api) {
            /* We've found our API. Pass along argc and argv skipping the program name and API */
            app.argc=argc-2;
            app.argv=&argv[2];
            return api->apiHandlerFunc(&app, api->apiFunc);
        }
        else {
            printf("The API '%s' is invalid! Please provide a valid command.\n", argv[1]);
            return 1;
        }
    }

    for (i=0; i != app.apiListSize; i++) {
        free(app.apiList[i]);
    }

    RMH_Destroy(app.rmh);
    return 0;
}
