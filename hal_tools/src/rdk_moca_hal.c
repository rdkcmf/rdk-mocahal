#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rdk_moca_hal.h"


typedef enum ValidationType {
    GET_UINT32,
    SET_BOOL,
    SET_UINT32,
    GET_BOOL,
    GET_NET_STATUS,
    GET_STRING
} ValidationType;

#define INIT_MENU_ITEM(_type, _func) {\
    MenuItem *newItem=(MenuItem *)malloc(sizeof(MenuItem)); \
    memset(newItem, 0, sizeof(MenuItem)); \
    newItem->menu=&menu; \
    newItem->executeFunc=Execute_##_type; \
    newItem->name=#_func; \
    newItem->type=_type; \
    newItem->_type.function=_func; \
    menu.items[menu.count++]=newItem; \
}

typedef struct MenuItem {
    const struct Menu* menu;
    const char *name;
    RMH_Result (*executeFunc)(RMH_Handle rmh, struct MenuItem *menuItem);
    ValidationType type;
    union {
        struct {
            RMH_Result (*function)(RMH_Handle rmh, uint32_t* response);
        } GET_UINT32;
        struct {
             RMH_Result (*function)(RMH_Handle rmh, bool* response);
        } GET_BOOL;
        struct {
             RMH_Result (*function)(RMH_Handle rmh, uint8_t* responseBuf, const size_t responseBufSize);
        } GET_STRING;
        struct {
            RMH_Result (*function)(RMH_Handle handle, RMH_NetworkStatus* response);
        } GET_NET_STATUS;


        struct {
            RMH_Result (*function)(RMH_Handle rmh, uint32_t value);
        } SET_UINT32;
        struct {
             RMH_Result (*function)(RMH_Handle rmh, bool value);
        } SET_BOOL;
    };
} MenuItem;

typedef struct Menu {
    MenuItem *items[2048];
    uint32_t count;
    bool interactive;
    int argc;
    char **argv;
} Menu;



static
RMH_Result ReadLine(const char *prompt, const Menu *menu, char *buf, const uint32_t bufSize) {
    char *input=NULL;
    if (menu->interactive) {
        if (prompt)
            printf("%s", prompt);
        input=fgets(buf, bufSize, stdin);
        if (input)
            input[strcspn(input, "\n")] = '\0';
    }
    else {
        input=(menu->argc >= 1) ? menu->argv[0] : NULL;
    }

    return (input && strlen(input)) ? RMH_SUCCESS : RMH_FAILURE;
}

static
RMH_Result ReadUint32(const Menu *menu, uint32_t *value) {
    char input[16];
    if (ReadLine("Enter your choice (number): ", menu, input, sizeof(input)) == RMH_SUCCESS) {
        char *end;
        *value=strtol(input, &end, 10);
        if (end != input && *end == '\0' && errno != ERANGE) {
            return RMH_SUCCESS;
        }
    }
    printf("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result ReadBool(const Menu *menu, bool *value) {
    char input[16];
    if (ReadLine("Enter your choice (TRUE,FALSE or 1,0): ", menu, input, sizeof(input)) == RMH_SUCCESS) {
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
RMH_Result ReadString(const Menu *menu, char *buf, const uint32_t bufSize) {
    if (ReadLine("Enter your choice: ", menu, buf, bufSize) != RMH_SUCCESS) {
        printf("Bad input. Please provide a valid string.\n");
    }
    return RMH_FAILURE;
}


static
RMH_Result Execute_GET_UINT32(RMH_Handle rmh, MenuItem *menuItem) {
    uint32_t response=0;
    RMH_Result ret = menuItem->GET_UINT32.function(rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%u\n", response);
    }
    return ret;
}

static
RMH_Result Execute_GET_BOOL(RMH_Handle rmh, MenuItem *menuItem) {
    bool response=0;
    RMH_Result ret = menuItem->GET_BOOL.function(rmh, &response);
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response ? "TRUE" : "FALSE");
    }
    return ret;
}

static
RMH_Result Execute_SET_BOOL(RMH_Handle rmh, MenuItem *menuItem) {
    bool value;
    RMH_Result ret=ReadBool(menuItem->menu, &value);
    if (ret == RMH_SUCCESS) {
        ret = menuItem->SET_BOOL.function(rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %s\n", value ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result Execute_SET_UINT32(RMH_Handle rmh, MenuItem *menuItem) {
    uint32_t value;
    RMH_Result ret=ReadUint32(menuItem->menu, &value);
    if (ret == RMH_SUCCESS) {
        ret = menuItem->SET_UINT32.function(rmh, value);
        if (ret == RMH_SUCCESS) {
            printf("Success setting to %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result Execute_GET_STRING(RMH_Handle rmh, MenuItem *menuItem) {
    char response[256];
    RMH_Result ret = menuItem->GET_STRING.function(rmh, response, sizeof(response));
    if (ret == RMH_SUCCESS) {
        printf("%s\n", response);
    }
    return ret;
}

static
RMH_Result Execute_GET_NET_STATUS(RMH_Handle rmh, MenuItem *menuItem) {
    RMH_NetworkStatus response;

    RMH_Result ret = menuItem->GET_NET_STATUS.function(rmh, &response);
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


static MenuItem *
FindMenuItem(const char *apiName, Menu *menu) {
    uint32_t i;
    uint32_t offset=0;
    if(strncmp(apiName, "RMH_", 4) != 0) {
        /* The user did not provide "RMH_" as part of the API name. Skip this when checking the menu */
        offset = 4;
    }

    for (i=0; i != menu->count; i++) {
        if(strncmp(apiName, menu->items[i]->name+offset,  strlen(menu->items[i]->name)-offset) == 0) {
            return menu->items[i];
        }
    }

  return NULL;
}

static
void Execute(RMH_Handle rmh, MenuItem *menuItem) {
    RMH_Result ret;
    printf("\n-----------------------------------------------------------------------------------------\n");
    printf("%s:\n", menuItem->name);
    ret=menuItem->executeFunc(rmh, menuItem);
    if (ret != RMH_SUCCESS) {
        printf("**Error %s!\n", RMH_ResultToString(ret));
    }
    printf("-----------------------------------------------------------------------------------------\n");
}

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

void RunMenu(RMH_Handle rmh, Menu *menu) {
    char inputBuf[8];
    char *input;
    uint32_t option;
    uint32_t i;

    while(true) {
        printf("\n\n");
        for (i=0; i != menu->count; i++) {
            printf("%02d. %s\n", i+1, menu->items[i]->name);
        }
        printf("%02d. %s\n", i+1, "Exit");

        if (ReadUint32(menu, &option)==RMH_SUCCESS) {
            if (option != 0) {
                if ( option <= menu->count ) {
                    option--;
                    Execute(rmh, menu->items[option]);
                    continue;
                }
                else if ( option == menu->count+1 ) {
                    printf("Exiting...\n");
                    return;
                }
            }
            printf("**Error: Invalid selection\n");
        }
    }
}

void DestroyMenu(Menu *menu) {
    uint32_t i;
    for (i=0; i != menu->count; i++) {
        free(menu->items[i]);
    }
}

int main(int argc, char *argv[])
{
    RMH_Handle rmh;
    Menu menu;

    memset(&menu, 0, sizeof(menu));
    menu.interactive = (argc <= 1);
    rmh=RMH_Initialize();
    RMH_RegisterEventCallback(rmh, EventCallback, (void*)0x12345678, LINK_STATUS_CHANGED | MOCA_VERSION_CHANGED);

    INIT_MENU_ITEM(GET_UINT32, RMH_GetLinkStatus);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetNumNodesInNetwork);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetNodeId);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetNetworkControllerNodeId);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetBackupNetworkControllerNodeId);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetRFChannel);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetLinkUptime);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetLOF);
    INIT_MENU_ITEM(GET_UINT32, RMH_GetBeaconPowerReduction);

    INIT_MENU_ITEM(GET_BOOL, RMH_GetBeaconPowerReductionEnabled);
    INIT_MENU_ITEM(GET_BOOL, RMH_GetPrivacyEnabled);
    INIT_MENU_ITEM(GET_BOOL, RMH_GetPreferredNC);
    INIT_MENU_ITEM(GET_BOOL, RMH_GetQAM256Capable);

    INIT_MENU_ITEM(GET_STRING, RMH_GetActiveMoCAVersion);
    INIT_MENU_ITEM(GET_STRING, RMH_GetHigestMoCAVersion);
    INIT_MENU_ITEM(GET_STRING, RMH_GetMacString);
    INIT_MENU_ITEM(GET_STRING, RMH_GetNetworkControllerMacString);
    INIT_MENU_ITEM(GET_STRING, RMH_GetPassword);
    INIT_MENU_ITEM(GET_STRING, RMH_GetSoftwareVersion);

    INIT_MENU_ITEM(GET_NET_STATUS, RMH_GetNetworkStatusTx);
    INIT_MENU_ITEM(GET_NET_STATUS, RMH_GetNetworkStatusRx);

    INIT_MENU_ITEM(SET_BOOL, RMH_SetPreferredNC);
    INIT_MENU_ITEM(SET_BOOL, RMH_SetPrivacyEnabled);
    INIT_MENU_ITEM(SET_BOOL, RMH_SetBeaconPowerReductionEnabled);

    INIT_MENU_ITEM(SET_UINT32, RMH_SetBeaconPowerReduction);

    if (menu.interactive) {
       RunMenu(rmh, &menu);
    }
    else {
        uint32_t i;
        MenuItem *api=FindMenuItem(argv[1], &menu);
        if (api) {
            /* We've found our API. Pass along argc and argv skipping the program name and API */
            menu.argc=argc-2;
            menu.argv=&argv[2];
            return api->executeFunc(rmh, api);
        }
        else {
            printf("The API '%s' is invalid! Please provide a valid command.\n", argv[1]);
            return 1;
        }
    }

    DestroyMenu(&menu);
    RMH_Destroy(rmh);
    return 0;
}


