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

/* Define for strcasestr */
#define _GNU_SOURCE

#include <signal.h>
#include <stdarg.h>
#include "rmh_app.h"

#define MAX_DISPLAY_COLUMN_WIDTH 80
#define RMH_PrintMsgWrapped(prefix, fmt, ...)    RMHApp_PrintWrapped(app, RMH_LOG_MESSAGE, __FUNCTION__, __LINE__, prefix, fmt, ##__VA_ARGS__);


/***********************************************************
 * Search Functions
 ***********************************************************/
static
const RMHApp_API* RMHApp_FindHandler(const RMHApp *app, const char *apiName) {
    uint32_t i;
    for (i=0; i != app->handledAPIs.apiListSize; i++) {
        if (strcasecmp(app->handledAPIs.apiList[i].apiName, apiName) == 0) {
            return &app->handledAPIs.apiList[i];
        }
    }

    /* We didn't find it on our first pass. Check the alias'' */
    for (i=0; i != app->handledAPIs.apiListSize; i++) {
        if (!app->handledAPIs.apiList[i].apiAlias || strlen(app->handledAPIs.apiList[i].apiAlias) == 0) continue;

        char* alias=strdup(app->handledAPIs.apiList[i].apiAlias);
        if (!alias) continue;
        char *token = strtok(alias, ",");
        while(token) {
            if (strcasecmp(token, apiName) == 0) {
                free(alias);
                return &app->handledAPIs.apiList[i];
            }
            token = strtok(NULL, ",");
        }
        free(alias);
    }
    return NULL;
}

static
const RMH_API* RMHApp_FindAPI(const RMHApp *app, const char *apiName) {
    uint32_t i;
    for (i=0; i != app->allAPIs->apiListSize; i++) {
        if (strcasecmp(app->allAPIs->apiList[i]->apiName, apiName) == 0) {
            return app->allAPIs->apiList[i];
        }
    }

    for (i=0; i != app->local.apiListSize; i++) {
        if (strcasecmp(app->local.apiList[i]->apiName, apiName) == 0) {
            return app->local.apiList[i];
        }
    }
    return NULL;
}

static
RMH_Result RMHApp_FindSimilarAPIs(const RMHApp *app, const char *searchString, RMH_APIList *closeMatch) {
    uint32_t i, j;
    if (closeMatch) {
        closeMatch->apiListSize=0;
        for (i=0; i != app->allAPIs->apiListSize; i++) {
            RMH_API* api=app->allAPIs->apiList[i];
            if (strcasestr(api->apiName, searchString) != NULL) {
                closeMatch->apiList[closeMatch->apiListSize++]=api;
            }
            else if (strcasestr(api->apiDefinition, searchString) != NULL) {
                closeMatch->apiList[closeMatch->apiListSize++]=api;
            }
            else if (strcasestr(api->apiDescription, searchString) != NULL) {
                closeMatch->apiList[closeMatch->apiListSize++]=api;
            }
            else if (strcasestr(api->tags, searchString) != NULL) {
                closeMatch->apiList[closeMatch->apiListSize++]=api;
            }
            else {
                for (j=0; j != api->apiNumParams; j++) {
                    if (strcasestr(api->apiParams[j].desc, searchString) != NULL) {
                        closeMatch->apiList[closeMatch->apiListSize++]=api;
                    }
                }
            }
        }
    }
    return (closeMatch && closeMatch->apiListSize) ? RMH_SUCCESS : RMH_FAILURE;
}

/***********************************************************
 * Print Functions
 ***********************************************************/
static
void RMHApp_PrintWrapped(RMHApp *app, const RMH_LogLevel level, const char *filename, const uint32_t lineNumber, const char *prefix, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *string;
    vasprintf (&string, format, args);

    //char* string=strdup(inStr);
    if (string) {
        char *lastBreak = string;
        char *printEnd = string;
        char *printStart=NULL;
        char *lastFoundSpace=NULL;
        int length = strlen(string);
        int maxWidth = MAX_DISPLAY_COLUMN_WIDTH - strlen(prefix);

        while(length>0) {
            lastFoundSpace=NULL;
            printStart=NULL;
            while (true) {
                if( !printStart && *printEnd != ' ' ) printStart = printEnd;
                if (*printEnd == '\0' || *printEnd == '\n') break;

                if( *printEnd == ' ' ) lastFoundSpace=printEnd;
                if ( printStart && lastFoundSpace && (printEnd >= printStart+maxWidth)) {
                    printEnd = lastFoundSpace; /* Found a wrap point, back up to last space */
                    break;
                }
                printEnd++;
            }

            *printEnd++ = '\0';
            if (printStart) RMH_PrintMsg("%s%s\n", prefix, printStart);
            length -= printEnd-lastBreak;
            lastBreak = printEnd;
        }
        free(string);
    }

    va_end(args);
}

static
RMH_Result RMHApp_PrintHelp(RMHApp *app) {
    RMH_PrintMsg("usage: rmh [-?|-h|--help] [-t|--trace] [-d|--debug] [-s|--search] [-l|--list]\n");
    RMH_PrintMsg("           <API> [<args>]\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("   --help            Print this help message\n");
    RMH_PrintMsg("   --list            Print a list of all available RMH APIs\n");
    RMH_PrintMsg("   --debug           Monitor MoCA driver level debug logs\n");
    RMH_PrintMsg("   --trace           Enable API trace messages\n");
    RMH_PrintMsg("   --search          Print a menu of all APIs matching the string passed as <API>\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("'rmh --help <API>' will show detailed information about what that API does\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("You can run 'rmh' with no parameters to see a menu of available APIs.\n");
    RMH_PrintMsg("\n");
    RMH_PrintMsg("\n");
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp_PrintAPIHelp(RMHApp *app, const RMH_API* api) {
    int i;
    bool first = true;
    char rmh_args[512];
    int rmh_args_written =0;
    const RMHApp_API *apiHandler=RMHApp_FindHandler(app, api->apiName);
    rmh_args[0] = '\0';

    RMH_PrintMsg("****************************************\n");
    RMH_PrintMsg("API:\n");
    RMH_PrintMsg("  %s\n\n", api->apiName);

    RMH_PrintMsg("ALIAS:\n");
    if (apiHandler && apiHandler->apiAlias) {
        RMH_PrintMsg("  %s\n\n", apiHandler->apiAlias);
    }
    else {
        RMH_PrintMsg("  None\n\n");
    }

    if (api->apiDefinition) {
        RMH_PrintMsg("DECLARATION:\n");
        RMH_PrintMsg("  %s\n\n", api->apiDefinition);
    }

    if (api->apiDescription) {
        RMH_PrintMsg("DESCRIPTION:\n");
        RMH_PrintMsgWrapped("  ", "%s\n\n", api->apiDescription);
    }

    first = true;
    for (i=0; i != api->apiNumParams; i++) {
        if (api->apiParams[i].direction == RMH_INPUT_PARAM) {
            if (first) {
                RMH_PrintMsg("INPUT PARAMETERS:\n");
                first = false;
            }
            RMH_PrintMsg("  <%s>\n", api->apiParams[i].name);
            RMH_PrintMsgWrapped("    ", "%s\n\n", api->apiParams[i].desc);
            if ((rmh_args_written != 0 || strstr(api->apiParams[i].type, "RMH_Handle") == NULL) &&
                (sizeof(rmh_args)/sizeof(rmh_args[0]) - rmh_args_written > 0))
                {
                rmh_args_written += sprintf( &rmh_args[rmh_args_written], "<%s> ", api->apiParams[i].name);
            }
        }
    }
    first = true;
    for (i=0; i != api->apiNumParams; i++) {
        if (api->apiParams[i].direction == RMH_OUTPUT_PARAM) {
            if (first) {
                RMH_PrintMsg("OUTPUT PARAMETERS:\n");
                first = false;
            }
            RMH_PrintMsg("  <%s>\n", api->apiParams[i].name);
            RMH_PrintMsgWrapped("    ", "%s\n\n", api->apiParams[i].desc);
        }
    }

    if (apiHandler) {
        RMH_PrintMsg("USAGE:\n");
        RMH_PrintMsg("  rmh %s %s\n\n", api->apiName, rmh_args);
    }
    RMH_PrintMsg("****************************************\n\n\n");
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp_PrintList(RMHApp *app, const RMH_APIList *list, const char *exitString) {
    uint32_t i;
    RMH_PrintMsg("\n\n");
    RMH_PrintMsg("%02d. %s\n", 1, exitString);
    if (list) {
        for (i=0; i != list->apiListSize; i++) {
            RMH_PrintMsg("%02d. %s\n", i+2, list->apiList[i]->apiName);
        }
    }
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp_PrintTagList(RMHApp *app, const RMH_APITagList *tagList) {
    uint32_t i;
    RMH_PrintMsg("\n\n");
    RMH_PrintMsg("%02d. %s\n", 1, "Exit");
    RMH_PrintMsg("%02d. %s\n", 2, "All APIs");
    RMH_PrintMsg("%02d. %s\n", 3, "SoC Unimplemented APIs");
    RMH_PrintMsg("%02d. %s\n", 4, "RMH Local APIs");
    if (tagList) {
        for (i=0; i != tagList->tagListSize; i++) {
            RMH_PrintMsg("%02d. %s\n", i+5, tagList->tagList[i].apiListName);
        }
    }
    return RMH_SUCCESS;
}



/***********************************************************
 * Execution Functions
 ***********************************************************/
static RMHApp* gIntApp=NULL;
static
void RMHApp_CleanupOnInt(int sig) {
    if (gIntApp) {
        RMH_Log_SetDriverLevel(gIntApp->rmh, gIntApp->driverLogLevel);
        RMH_Destroy(gIntApp->rmh);
    }
    exit(0);
}

static
RMH_Result RMHApp_ExecuteMonitorDriverDebug(RMHApp *app) {
    gIntApp=app;
    signal(SIGINT, RMHApp_CleanupOnInt);

    RMH_PrintMsg("Monitoring for callbacks and MoCA logs. Press enter to exit...\n");
    if (RMH_SetEventCallbacks(app->rmh, RMH_EVENT_DRIVER_PRINT) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set callback events! Unable to monitor\n");
        return RMH_FAILURE;
    }

    if (RMH_Log_GetDriverLevel(app->rmh, &app->driverLogLevel) != RMH_SUCCESS) {
        RMH_PrintWrn("Failed to get driver log level. Assuming default!\n");
        app->driverLogLevel = RMH_LOG_DEFAULT;
    }

    if (RMH_Log_SetDriverLevel(app->rmh, app->driverLogLevel | RMH_LOG_DEBUG) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set log level to include RMH_LOG_DEBUG!\n");
        return RMH_FAILURE;
    }
    fgetc(stdin);

    RMH_Log_SetDriverLevel(app->rmh, app->driverLogLevel);
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp_ExecuteAPIList(RMHApp *app, const RMH_APIList *list, const char *exitString) {
    uint32_t option;
    bool helpRequested = false;

    RMHApp_PrintList(app, list, exitString);
    while(true) {
        if (RMHApp_ReadMenuOption(app, &option, true, &helpRequested) == RMH_SUCCESS) {
            if ( option > 0) {
                if ( option == 1 ) break;
                option-=2;

                if (list && option<list->apiListSize) {
                    RMH_Result ret;
                    const RMH_API* api = list->apiList[option];
                    if (app->argHelpRequested || helpRequested) {
                        RMHApp_PrintAPIHelp(app, api);
                        continue;
                    }
                    RMH_PrintMsg(" ----------------------------------------------------------------------------\n");
                    RMH_PrintMsg("|%s\n", api->apiName);
                    RMH_PrintMsg("|----------------------------------------------------------------------------\n");
                    app->appPrefix="|  ";

                    const RMHApp_API *apiHandler=RMHApp_FindHandler(app, api->apiName);
                    if (apiHandler == NULL) {
                        RMH_PrintErr("The RMH library supports '%s' however it has not been exposed in RMH!\n", api->apiName);
                        RMH_PrintErr("Please add a handler function for this API in the rmh test by using SET_API_HANDLER()\n");
                        ret = RMH_FAILURE;
                    }
                    else {
                        ret = apiHandler->apiHandlerFunc(app, apiHandler->apiFunc);
                    }
                    if (ret != RMH_SUCCESS) {
                        RMH_PrintErr("Failed with error: %s!\n", RMH_ResultToString(ret));
                    }
                    app->appPrefix=NULL;
                    RMH_PrintMsg(" ----------------------------------------------------------------------------\n\n\n");
                    continue;
                }
            }
            RMH_PrintErr("Invalid selection\n");
        }
        RMHApp_PrintList(app, list, exitString);

    }
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp_ExecuteCommand(RMHApp *app) {
    const RMH_API* api;
    if (app->argPrintMatch) {
        RMH_APIList closeMatch;
        RMHApp_FindSimilarAPIs(app, app->argRunCommand, &closeMatch);
        if (closeMatch.apiListSize == 0) {
            RMH_PrintErr("RMH has no API named '%s' and no other API seem to be related to this\n", app->argRunCommand);
            return RMH_FAILURE;
        }

        /* We're done with the command, transition to interactive mode */
        app->argRunCommand=NULL;
        return RMHApp_ExecuteAPIList(app, &closeMatch, "Exit");
    }


    const RMHApp_API* apiHandler=RMHApp_FindHandler(app, app->argRunCommand);
    if (app->argHelpRequested) {
        api = RMHApp_FindAPI(app, apiHandler ? apiHandler->apiName : app->argRunCommand);
        if (api == NULL) {
            RMH_PrintErr("RMH has no API named '%s'\n", app->argRunCommand);
            return RMH_FAILURE;
        }
        return RMHApp_PrintAPIHelp(app, api);
    }
    else {
        if (apiHandler == NULL) {
            api=RMHApp_FindAPI(app, app->argRunCommand);
            if (api == NULL) {
                RMH_PrintErr("RMH has no API named '%s'\n", app->argRunCommand);
                return RMH_FAILURE;
            }
            RMH_PrintErr("RMH has supports the API '%s' however it has not been exposed in RMH. Please add a handler function for this API in the rmh test by using SET_API_HANDLER()\n", app->argRunCommand);
            return RMH_FAILURE;
        }
        return apiHandler->apiHandlerFunc(app, apiHandler->apiFunc);
    }
}

static
RMH_Result RMHApp_ExecuteTagList(RMHApp *app, const RMH_APITagList *tagList) {
    uint32_t option;

    RMH_SetEventCallbacks(app->rmh, RMH_EVENT_LINK_STATUS_CHANGED | RMH_EVENT_MOCA_VERSION_CHANGED | RMH_EVENT_API_PRINT);
    RMHApp_PrintTagList(app, tagList);
    while(true) {
        if (RMHApp_ReadMenuOption(app, &option, false, NULL)==RMH_SUCCESS){
            if ( option > 0 ) {
                if ( option == 1 )
                    break;
                else if (option==2) {
                    RMHApp_ExecuteAPIList(app, app->allAPIs, "Go back");
                }
                else if (option==3) {
                    RMHApp_ExecuteAPIList(app, app->unimplementedAPIs, "Go back");
                }
                else if (option==4) {
                    RMHApp_ExecuteAPIList(app, &app->local, "Go back");
                }
                else {
                    option-=5;
                    if (tagList && option < tagList->tagListSize) {
                        RMHApp_ExecuteAPIList(app, &tagList->tagList[option], "Go back");
                    }
                }
            }
            RMH_PrintErr("Invalid selection\n");
        }
        RMHApp_PrintTagList(app, tagList);
    }
    return RMH_SUCCESS;
}

/***********************************************************
 * Util Functions
 ***********************************************************/
static
void RMHApp_EventCallback(const enum RMH_Event event, const struct RMH_EventData *eventData, void* userContext){
    RMHApp *app=(RMHApp *)userContext;
    switch(event) {
    case RMH_EVENT_LINK_STATUS_CHANGED:
        RMH_PrintMsg("\n%p: Link status changed to %s\n", userContext, RMH_LinkStatusToString(eventData->RMH_EVENT_LINK_STATUS_CHANGED.status));
        break;
    case RMH_EVENT_MOCA_VERSION_CHANGED:
        RMH_PrintMsg("\n%p: MoCA Version Changed to %s\n", userContext, RMH_MoCAVersionToString(eventData->RMH_EVENT_MOCA_VERSION_CHANGED.version));
        break;
    case RMH_EVENT_API_PRINT:
        RMH_PrintMsg("%s", eventData->RMH_EVENT_API_PRINT.logMsg);
        break;
    case RMH_EVENT_DRIVER_PRINT:
        RMH_PrintMsg("%s", eventData->RMH_EVENT_DRIVER_PRINT.logMsg);
        break;
    default:
        RMH_PrintWrn("Unhandled MoCA event %u!\n", event);
        break;
    }
}

static
RMH_Result RMHApp_ParseOptions(RMHApp *app) {
    RMHApp_ReadNextArg(app); /* First read to drop program name */

    do{
        const char* option=RMHApp_ReadNextArg(app);
        if (!option) break;

        if (option[0] == '-') {
            if (strcmp(option, "-d") == 0 || strcmp(option, "--debug") == 0) {
                app->argMonitorDriverDebug = true;
            } else if (strcmp(option, "-t") == 0 || strcmp(option, "--trace") == 0) {
                app->apiLogLevel |= RMH_LOG_TRACE;
            } else if (strcmp(option, "-?") == 0 || strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
                app->argHelpRequested = true;
            } else if (strcmp(option, "-s") == 0 || strcmp(option, "--search") == 0) {
                app->argPrintMatch = true;
            } else if (strcmp(option, "-l") == 0 || strcmp(option, "--list") == 0) {
                app->argPrintApis = true;
            }
            else {
                RMH_PrintWrn("Unknown option '%s'! Skipping\n", option);
                break;
            }
        }
        else {
            app->argRunCommand = option;
            break;
        }
    } while(true);

    return RMH_SUCCESS;
}



/***********************************************************
 * Main
 ***********************************************************/
int main(int argc, char *argv[])
{
    RMHApp appStr;
    RMHApp* app=&appStr;
    RMH_Result result;

    memset(app, 0, sizeof(*app));
    app->apiLogLevel = RMH_LOG_DEFAULT;
    app->argc=argc;
    app->argv=argv;
    RMHApp_ParseOptions(app);

    app->rmh=RMH_Initialize(RMHApp_EventCallback, app);
    if (!app->rmh){
        RMH_PrintErr("Failed in RMH_Initialize!\n");
        return RMH_FAILURE;
    }

    if (RMH_GetAllAPIs(app->rmh, &app->allAPIs) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get list of all APIs!\n");
        return RMH_FAILURE;
    }

    if (RMH_Log_SetAPILevel(app->rmh, app->apiLogLevel ) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set the log level!\n");
    }

    if (RMH_SetEventCallbacks(app->rmh, RMH_EVENT_API_PRINT) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set event callbacks!\n");
    }

    if (RMH_GetUnimplementedAPIs(app->rmh, &app->unimplementedAPIs) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get list of all unimplemented APIs!\n");
    }

    if (RMH_GetAPITags(app->rmh, &app->rmhAPITags) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get list of all API tags!\n");
    }

    RMHApp_RegisterAPIHandlers(app);

    if (app->argRunCommand) {
        result = RMHApp_ExecuteCommand(app);
    }
    else if (app->argPrintApis) {
        uint32_t i;
        for (i=0; i != app->allAPIs->apiListSize; i++) {
            RMH_PrintMsg("%s\n", app->allAPIs->apiList[i]->apiName);
            if (app->argHelpRequested) {
                RMH_PrintMsgWrapped("  ", "%s\n\n", app->allAPIs->apiList[i]->apiDescription);
            }
        }
        result=RMH_SUCCESS;
    }
    else if (app->argMonitorDriverDebug) {
        result = RMHApp_ExecuteMonitorDriverDebug(app);
    }
    else if (app->argHelpRequested) {
        result=RMHApp_PrintHelp(app);
    }
    else {
        result = RMHApp_ExecuteTagList(app, app->rmhAPITags);
    }

    RMH_Destroy(app->rmh);
    return result;
}
