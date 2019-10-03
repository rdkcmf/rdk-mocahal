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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "rmh_app.h"

#define RDK_FILE_PATH_PREVENT_MOCA_START        "/opt/sysproperties/mocakillswitchenable"
#define RDK_FILE_PATH_PREVENT_MOCA_START2       "/opt/mocakillswitchenable"

#undef AS
#define AS(x,y) #x,
const char * const RMH_LogLevelStr[] = { ENUM_RMH_LogLevel };


/***********************************************************
 * Local API Functions
 ***********************************************************/
RMH_Result RMHApp_DisableDriverDebugLogging(const RMHApp *app) {
    char fileName[128];
    RMH_LogLevel logLevel;

    if (RMH_Log_GetDriverFilename(app->rmh, fileName, sizeof(fileName)) == RMH_SUCCESS) {
        RMH_PrintMsg("MoCA stopping logging to %s\n", fileName);

        if (RMH_Log_SetDriverFilename(app->rmh, NULL) != RMH_SUCCESS) {
            RMH_PrintErr("Failed to stop logging!\n");
            return RMH_FAILURE;
        }
    }

    if (RMH_Log_GetDriverLevel(app->rmh, &logLevel) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get driver log level!\n");
        return RMH_FAILURE;
    }

    if ((~logLevel & RMH_LOG_DEBUG) == RMH_LOG_DEBUG) {
        RMH_PrintMsg("MoCA driver debug logging already disabled\n");
        return RMH_SUCCESS;
    }

    if (RMH_Log_SetDriverLevel(app->rmh, (logLevel & ~RMH_LOG_DEBUG)) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set log level to exclude RMH_LOG_DEFAULT!\n");
        return RMH_FAILURE;
    }
    RMH_PrintMsg("MoCA driver debug logging disabled\n");

    return RMH_SUCCESS;
}

RMH_Result RMHApp_EnableDriverDebugLogging(const RMHApp *app) {
    char logFileName[1024];
    RMH_LogLevel logLevel;

    if (RMH_Log_GetDriverFilename(app->rmh, logFileName, sizeof(logFileName)) == RMH_SUCCESS) {
        RMH_PrintMsg("Logging to '%s'\n", logFileName);
    }

    if (RMH_Log_GetDriverLevel(app->rmh, &logLevel) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get driver log level!\n");
        return RMH_FAILURE;
    }

    if ((logLevel & RMH_LOG_DEBUG) == RMH_LOG_DEBUG) {
        RMH_PrintMsg("MoCA driver debug logging already enabled\n");
        return RMH_SUCCESS;
    }

    if (RMH_Log_SetDriverLevel(app->rmh, (logLevel | RMH_LOG_DEBUG)) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to set driver log level to include RMH_LOG_DEBUG!\n");
        return RMH_FAILURE;
    }
    RMH_PrintMsg("MoCA driver debug logging enabled\n");
    return RMH_SUCCESS;
}

RMH_Result RMHApp_DriverLogForever(const RMHApp *app) {
    char logFileName[1024];
    RMH_LogLevel logLevel;

    if (RMH_Log_GetDriverFilename(app->rmh, logFileName, sizeof(logFileName)) == RMH_SUCCESS) {
        RMH_PrintMsg("Stop logging to '%s'.\n", logFileName);
    }

    if (RMH_Log_CreateDriverFile(app->rmh, logFileName, sizeof(logFileName)) != RMH_SUCCESS) {
        RMH_PrintErr("Unable to create dedicated log file!\n");
        return RMH_FAILURE;
    }

    if (RMH_Log_SetDriverFilename(app->rmh, logFileName) != RMH_SUCCESS) {
        RMH_PrintErr("Unable to start logging in %s!\n", logFileName);
        return RMH_FAILURE;
    }

    if (RMH_Log_GetDriverLevel(app->rmh, &logLevel) != RMH_SUCCESS) {
        RMH_PrintErr("Failed to get driver log level!\n");
        return RMH_FAILURE;
    }

    if ((logLevel & RMH_LOG_DEBUG) != RMH_LOG_DEBUG) {
        if (RMH_Log_SetDriverLevel(app->rmh, (logLevel | RMH_LOG_DEBUG)) != RMH_SUCCESS) {
            RMH_PrintErr("Failed to set log level to RMH_LOG_DEBUG!\n");
            return RMH_FAILURE;
        }
    }
    RMH_PrintMsg("Started logging enabled to '%s'.\n", logFileName);
    return RMH_SUCCESS;
}

RMH_Result RMHApp_Stop(const RMHApp *app) {
    bool started;

    RMH_Result ret=RMH_Self_GetEnabled(app->rmh, &started);
    if (!started) {
        RMH_PrintErr("MoCA is already stopped\n");
        return RMH_INVALID_INTERNAL_STATE;
    }

    ret=RMH_Self_SetEnabled(app->rmh, false);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("MoCA successfully stopped\n");
    }
    return ret;
}

RMH_Result RMHApp_Start(const RMHApp *app) {
    bool started = false;

    RMH_Result ret=RMH_Self_GetEnabled(app->rmh, &started);
    if (started) {
        RMH_PrintErr("MoCA is already started\n");
        return RMH_INVALID_INTERNAL_STATE;
    }

    if (access(RDK_FILE_PATH_PREVENT_MOCA_START, F_OK ) != -1) {
        RMH_PrintWrn("Refusing to start MoCA because the file %s exists!\n", RDK_FILE_PATH_PREVENT_MOCA_START);
    }
    else if (access(RDK_FILE_PATH_PREVENT_MOCA_START2, F_OK ) != -1) {
        RMH_PrintWrn("Refusing to start MoCA because the file %s exists!\n", RDK_FILE_PATH_PREVENT_MOCA_START2);
    }
    else {
        ret=RMH_Self_SetEnabled(app->rmh, true);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("MoCA successfully started\n");
        }
    }

    return ret;
}

/***********************************************************
 * Input Fuctions
 ***********************************************************/
const char * RMHApp_ReadNextArg(RMHApp *app) {
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
    if (app->argRunCommand == NULL) {
        if (prompt)
            RMH_PrintMsg("%s", prompt);
        input=fgets(buf, bufSize, stdin);
        if (input) {
            buf[strcspn(buf, "\n")] = '\0';
        }
    }
    else {
        input=RMHApp_ReadNextArg(app);
        if (input) {
            strncpy(buf, input, bufSize);
            buf[bufSize-1]='\0';
        }
    }

    return (input && strlen(input)) ? strlen(input) : 0;
}

RMH_Result RMHApp_ReadMenuOption(RMHApp *app, uint32_t *value, bool helpSupported, bool *helpRequested) {
    char input[32];
    if (ReadLine("Enter your choice (number): ", app, input, sizeof(input))) {
        char *end;
        *value=(uint32_t)strtol(input, &end, 10);
        if (end != input && errno != ERANGE) {
            if (*end == '\0') {
                if (helpRequested != NULL) *helpRequested=false;
                return RMH_SUCCESS;
            }
            else if (helpSupported && end[0] == '?' && end[1] == '\0') {
                if (helpRequested != NULL) *helpRequested=true;
                return RMH_SUCCESS;
            }
        }
    }
    RMH_PrintErr("Bad input. Please enter a valid unsigned number\n");
    return RMH_FAILURE;
}

static
RMH_Result RMHApp_ReadUint32(RMHApp *app, uint32_t *value) {
    char input[32];
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
RMH_Result RMHApp_ReadMAC(RMHApp *app, RMH_MacAddress_t* response) {
    char input[32];
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
RMH_Result RMHApp_ReadLogLevel(RMHApp *app, uint32_t *value) {
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
RMH_Result RMHApp_ReadInt32(RMHApp *app, int32_t *value) {
    char input[32];
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
RMH_Result RMHApp_ReadUint32Hex(RMHApp *app, uint32_t *value) {
    char input[32];
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
RMH_Result RMHApp_ReadBool(RMHApp *app, bool *value) {
    char input[32];
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
RMH_Result RMHApp_ReadString(RMHApp *app, char *buf, const uint32_t bufSize) {
    if (ReadLine("Enter your choice: ", app, buf, bufSize)) {
        return RMH_SUCCESS;
    }
    RMH_PrintErr("Bad input. Please provide a valid string.\n");
    return RMH_FAILURE;
}

static
RMH_Result RMHApp_ReadACAType(RMHApp *app, RMH_ACAType *value) {
    char input[32];
    char choices[128];
    snprintf(choices, sizeof(choices)/sizeof(choices[0]), "Enter your choice [EVM(%u),QUIET(%u)]: ", RMH_ACA_TYPE_EVM, RMH_ACA_TYPE_QUIET);

    if (ReadLine(choices, app, input, sizeof(input))) {
        int inputAsInt=(int)(input[0] - '0');
        if ((inputAsInt == RMH_ACA_TYPE_EVM) || (strcasecmp(input, "RMH_ACA_TYPE_EVM") == 0) || (strcasecmp(input, "EVM") == 0)) {
            *value=RMH_ACA_TYPE_EVM;
            return RMH_SUCCESS;
        }
        else if ((inputAsInt == RMH_ACA_TYPE_QUIET) || (strcasecmp(input, "RMH_ACA_TYPE_QUIET") == 0) || (strcasecmp(input, "QUIET") == 0)) {
            *value=RMH_ACA_TYPE_QUIET;
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input\n");
    return RMH_FAILURE;
}

static
RMH_Result RMHApp_ReadPowerMode(RMHApp *app, RMH_PowerMode *value) {
    char input[32];
    char choices[128];
    snprintf(choices, sizeof(choices)/sizeof(choices[0]), "Enter your choice [ACTIVE(%u),IDLE(%u),STANDBY(%u),SLEEP(%u)]: ", RMH_POWER_MODE_M0_ACTIVE, RMH_POWER_MODE_M1_IDLE, RMH_POWER_MODE_M2_STANDBY, RMH_POWER_MODE_M3_SLEEP);
    if (ReadLine(choices, app, input, sizeof(input))) {
        int inputAsInt=(int)(input[0] - '0');
        if ((inputAsInt == RMH_POWER_MODE_M0_ACTIVE) || (strcasecmp(input, "RMH_POWER_MODE_M0_ACTIVE") == 0) || (strcasecmp(input, "ACTIVE") == 0)) {
            *value=RMH_POWER_MODE_M0_ACTIVE;
            return RMH_SUCCESS;
        }
        else if ((inputAsInt == RMH_POWER_MODE_M1_IDLE) || (strcasecmp(input, "RMH_POWER_MODE_M1_IDLE") == 0) || (strcasecmp(input, "IDLE") == 0)) {
            *value=RMH_POWER_MODE_M1_IDLE;
            return RMH_SUCCESS;
        }
        else if ((inputAsInt == RMH_POWER_MODE_M2_STANDBY) || (strcasecmp(input, "RMH_POWER_MODE_M2_STANDBY") == 0) || (strcasecmp(input, "STANDBY") == 0)) {
            *value=RMH_POWER_MODE_M2_STANDBY;
            return RMH_SUCCESS;
        }
        else if ((inputAsInt == RMH_POWER_MODE_M3_SLEEP) || (strcasecmp(input, "RMH_POWER_MODE_M3_SLEEP") == 0) || (strcasecmp(input, "SLEEP") == 0)) {
            *value=RMH_POWER_MODE_M3_SLEEP;
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input\n");
    return RMH_FAILURE;
}

static
RMH_Result RMHApp_ReadPERMode(RMHApp *app, RMH_PERMode *value) {
    char input[32];
    if (ReadLine("Enter your choice (LEGACY,NPER,VLPER or 0,1,2): ", app, input, sizeof(input))) {
        if ((strcmp(input, "0") == 0) || (strcasecmp(input, "RMH_PER_MODE_LEGACY") == 0) || (strcasecmp(input, "LEGACY") == 0)) {
            *value=RMH_PER_MODE_LEGACY;
            return RMH_SUCCESS;
        }
        else if ((strcmp(input, "1") == 0) || (strcasecmp(input, "RMH_PER_MODE_NOMINAL") == 0) || (strcasecmp(input, "NPER") == 0)) {
            *value=RMH_PER_MODE_NOMINAL;
            return RMH_SUCCESS;
        }
        else if ((strcmp(input, "2") == 0) || (strcasecmp(input, "RMH_PER_MODE_VERY_LOW") == 0) || (strcasecmp(input, "VLPER") == 0)) {
            *value=RMH_PER_MODE_VERY_LOW;
            return RMH_SUCCESS;
        }
    }
    RMH_PrintErr("Bad input. Please use only LEGACY,NPER,VLPER or 0,1,2\n");
    return RMH_FAILURE;
}


/***********************************************************
 * API Handler Functions (Output Functions)
 ***********************************************************/
static
RMH_Result RMHApp__OUT_UINT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%u\n", response);
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_UINT32_HEX(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, uint32_t* response)) {
    uint32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("0x%08x\n", response);
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_INT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, int32_t* response)) {
    int32_t response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%d\n", response);
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_UINT32_ARRAY(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize, size_t* responseBufUsed)) {
    uint32_t responseBuf[256];
    size_t responseBufUsed;
    int i;

    RMH_Result ret = api(app->rmh, responseBuf, sizeof(responseBuf)/sizeof(responseBuf[0]), &responseBufUsed);
    if (ret == RMH_SUCCESS) {
        for (i=0; i < responseBufUsed; i++) {
            RMH_PrintMsg("[%02u] %u\n", i, responseBuf[i]);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_MAC_ARRAY(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_MacAddress_t* responseBuf, const size_t responseBufSize, size_t* responseBufUsed)) {
    RMH_MacAddress_t responseBuf[32];
    size_t responseBufUsed;
    char macStr[24];
    int i;

    RMH_Result ret = api(app->rmh, responseBuf, sizeof(responseBuf)/sizeof(responseBuf[0]), &responseBufUsed);
    if (ret == RMH_SUCCESS) {
        for (i=0; i < responseBufUsed; i++) {
            RMH_PrintMsg("[%02u] %s\n", i, RMH_MacToString(responseBuf[i], macStr, sizeof(macStr)/sizeof(macStr[0])));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_UINT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, uint32_t* response)) {
    uint32_t response=0;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%u\n", response);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_INT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, int32_t* response)) {
    int32_t response=0;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%d\n", response);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_FLOAT(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, float* response)) {
    float response=0;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%.03f\n", response);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_BOOL(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, bool* response)) {
    bool response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", response ? "TRUE" : "FALSE");
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_BOOL(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, bool* response)) {
    bool response=0;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%s\n", response ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_STRING(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    RMH_Result ret = api(app->rmh, response, sizeof(response));
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", response);
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_UINT8_ARRAY(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, uint8_t* responseArray, const size_t responseArraySize, size_t* responseArrayUsed)) {
    uint8_t responseBuf[1024];
    size_t responseBufUsed;
    int i,j;

    RMH_Result ret = api(app->rmh, responseBuf, sizeof(responseBuf)/sizeof(responseBuf[0]), &responseBufUsed);
    if (ret == RMH_SUCCESS) {
        for (i=0; i < responseBufUsed;) {
            RMH_PrintMsg("[%04u] ", i);
            for (j=0; j < 16 && i < responseBufUsed; j++,i++) {
                if (j == 7) {
                    RMH_PrintMsg("%02u   ", responseBuf[i]);
                }
                else {
                    RMH_PrintMsg("%02u ", responseBuf[i]);
                }
            }
            RMH_PrintMsg("\n");
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_STRING(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, char* responseBuf, const size_t responseBufSize)) {
    char response[256];
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, response, sizeof(response));
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%s\n", response);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_RMH_ENUM(RMHApp *app, const char* const (*api)(const RMH_Result)) {
    RMH_Result enumIndex;
    RMH_Result ret=RMHApp_ReadUint32(app, (uint32_t*)&enumIndex);
    if (ret == RMH_SUCCESS) {
        const char * str = api(enumIndex);
        if (str) {
            RMH_PrintMsg("%s\n", str);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_MAC(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_MacAddress_t* response)) {
    RMH_MacAddress_t response;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        char mac[24];
        RMH_PrintMsg("%s\n", RMH_MacToString(response, mac, sizeof(mac)/sizeof(mac[0])));
    }
    return ret;
}

static
RMH_Result RMHApp__IN_MAC(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const RMH_MacAddress_t value)) {
    RMH_MacAddress_t mac;
    RMH_Result ret=RMHApp_ReadMAC(app, &mac);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, mac);
        if (ret == RMH_SUCCESS) {
            char macStr[24];
            RMH_PrintMsg("Success passing to %s\n", RMH_MacToString(mac, macStr, sizeof(macStr)/sizeof(macStr[0])));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_MAC_OUT_UINT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response)) {
    RMH_MacAddress_t mac;
    uint32_t response;

    RMH_Result ret=RMHApp_ReadMAC(app, &mac);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, mac, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%u\n", response);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_MAC_OUT_MAC(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const RMH_MacAddress_t flowId, RMH_MacAddress_t* response)) {
    RMH_MacAddress_t mac;
    RMH_MacAddress_t response;

    RMH_Result ret=RMHApp_ReadMAC(app, &mac);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, mac, &response);
        if (ret == RMH_SUCCESS) {
            char mac[24];
            RMH_PrintMsg("%s\n", RMH_MacToString(response, mac, sizeof(mac)/sizeof(mac[0])));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_MAC(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, RMH_MacAddress_t* response)) {
    RMH_MacAddress_t response;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            char macStr[24];
            RMH_PrintMsg("%s\n", RMH_MacToString(response, macStr, sizeof(macStr)/sizeof(macStr[0])));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_POWER_MODE(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_PowerMode* response)) {
    RMH_PowerMode response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        char outStr[128];
        RMH_PrintMsg("%s\n", RMH_PowerModeToString(response, outStr, sizeof(outStr)));
    }
    return ret;
}

static
RMH_Result RMHApp__IN_POWER_MODE(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const RMH_PowerMode response)) {
    RMH_PowerMode mode;
    RMH_Result ret=RMHApp_ReadPowerMode(app, &mode);
    if (ret == RMH_SUCCESS) {
        RMH_Result ret = api(app->rmh, mode);
        if (ret == RMH_SUCCESS) {
            char outStr[128];
            RMH_PrintMsg("Success setting power mode to %s\n", RMH_PowerModeToString(mode, outStr, sizeof(outStr)));
        }
        else {
            RMH_PrintMsg("Failed with error %s\n", RMH_ResultToString(ret));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_BAND(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_Band* response)) {
    RMH_Band response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_BandToString(response));
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_LINK_STATUS(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_LinkStatus* response)) {
    RMH_LinkStatus response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_LinkStatusToString(response));
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_MOCA_VERSION(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_MoCAVersionToString(response));
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_OUT_MOCA_VERSION(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, RMH_MoCAVersion* response)) {
    RMH_MoCAVersion response=0;
    uint32_t nodeId;
    RMH_Result ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, nodeId, &response);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("%s\n", RMH_MoCAVersionToString(response));
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_LOGLEVEL(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_LogLevel* response)) {
    RMH_LogLevel response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        char outStr[128];
        RMH_PrintMsg("%s\n", RMH_LogLevelToString(response, outStr, sizeof(outStr)));
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_UINT32_NODELIST(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_NodeList_Uint32_t* response)) {
    RMH_NodeList_Uint32_t response;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        int i;
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                RMH_PrintMsg("NodeId:%u -- %u\n", i, response.nodeValue[i]);
            }
        }
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_UINT32_NODEMESH(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_NodeMesh_Uint32_t* response)) {
    RMH_NodeMesh_Uint32_t response;
    int i,j;

    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        char phyStrBegin[256];
        char *phyStrEnd=phyStrBegin+sizeof(phyStrBegin);
        char *phyStr=phyStrBegin;

        phyStr+=snprintf(phyStr, phyStrEnd-phyStr, "   ");
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                phyStr+=snprintf(phyStr, phyStrEnd-phyStr, "  %02u ", i);
            }
        }
        RMH_PrintMsg("%s\n", phyStrBegin);

        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                char *phyStr=phyStrBegin;
                RMH_NodeList_Uint32_t *nl = &response.nodeValue[i];
                phyStr+=snprintf(phyStr, phyStrEnd-phyStr, "%02u: ", i);
                for (j = 0; j < RMH_MAX_MOCA_NODES; j++) {
                    if (i == j) {
                        phyStr+=snprintf(phyStr, phyStrEnd-phyStr, " --  ");
                    } else if (response.nodePresent[j]) {
                        phyStr+=snprintf(phyStr, phyStrEnd-phyStr, "%04u ", nl->nodeValue[j]);
                    }
                }
                RMH_PrintMsg("%s\n", phyStrBegin);
            }
        }
    }
    else {
        RMH_PrintMsg("%s\n", RMH_ResultToString(ret));
    }

    return ret;
}

static
RMH_Result RMHApp__IN_BOOL (RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const bool value)) {
    bool value;
    RMH_Result ret=RMHApp_ReadBool(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing %s\n", value ? "TRUE" : "FALSE");
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=RMHApp_ReadUint32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_INT32(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const int32_t value)) {
    int32_t value;
    RMH_Result ret=RMHApp_ReadInt32(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing %d\n", value);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_UINT32_HEX(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t value)) {
    uint32_t value;
    RMH_Result ret=RMHApp_ReadUint32Hex(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing 0x%08x\n", value);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_STRING(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const char* value)) {
    char input[256];
    RMH_Result ret=RMHApp_ReadString(app, input, sizeof(input));
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, input);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing %s\n", input);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__IN_LOGLEVEL(RMHApp *app, RMH_Result (*api)(RMH_Handle rmh, const RMH_LogLevel response)) {
    RMH_LogLevel value;
    int i;

    for (i=0; (app->argRunCommand==NULL) && i != sizeof(RMH_LogLevelStr)/sizeof(RMH_LogLevelStr[0]); i++ ) {
        RMH_PrintMsg("%d. %s\n", i, RMH_LogLevelStr[i]);
    }
    RMH_Result ret=RMHApp_ReadLogLevel(app, &value);
    if (ret == RMH_SUCCESS) {
        ret = api(app->rmh, value);
        if (ret == RMH_SUCCESS) {
            RMH_PrintMsg("Success passing %u\n", value);
        }
    }
    return ret;
}

static
RMH_Result RMHApp__PRINT_STATUS(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const char*filename)) {
    RMH_Result ret = api(app->rmh, NULL);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("Success\n");
    }
    return ret;
}

static
RMH_Result RMHApp__HANDLE_ONLY(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle)) {
    RMH_Result ret = api(app->rmh);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("Success\n");
    }
    return ret;
}

static
RMH_Result RMHApp__LOCAL_HANDLE_ONLY(const RMHApp *app, RMH_Result (*api)(const RMHApp* app)) {
    return api(app);
}

static
RMH_Result RMHApp__GET_TABOO(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, uint32_t* start, uint32_t* mask)) {
    uint32_t start=0;
    uint32_t mask=0;
    RMH_Result ret = api(app->rmh, &start, &mask);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("Start channel: %u\n", start);
        RMH_PrintMsg("Channel Mask: 0x%08x\n", mask);
    }
    return ret;
}

static
RMH_Result RMHApp__SET_TABOO(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t start, const uint32_t mask)) {
    uint32_t start=0;
    uint32_t mask=0;

    RMH_Result ret=RMHApp_ReadUint32(app, &start);
    if (ret == RMH_SUCCESS) {
        RMH_Result ret=RMHApp_ReadUint32Hex(app, &mask);
        if (ret == RMH_SUCCESS) {
            ret = api(app->rmh, start, mask);
            if (ret == RMH_SUCCESS) {
                RMH_PrintMsg("Success setting Start:%u Channel Mask:0x%08x\n", start, mask);
            }
        }
    }

    return ret;
}

static
RMH_Result RMHApp__OUT_ACA_TYPE(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_ACAType* response)) {
    RMH_ACAType response=0;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_ACATypeToString(response));
    }
    return ret;
}

static
RMH_Result RMHApp__OUT_ACA_STATUS(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_ACAStatus* response)) {
    RMH_ACAStatus response;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_ACAStatusToString(response));
    }
    return ret;
}

static
RMH_Result RMHApp__REQUEST_ACA(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t channelNum, const uint32_t sourceNodeId, const uint32_t destinationNodeMask, const RMH_ACAType type)) {
    uint32_t channelNum;
    uint32_t sourceNodeId;
    uint32_t destinationNodeMask;
    RMH_ACAType type;
    RMH_Result ret;

    ret=RMHApp_ReadUint32(app, &channelNum);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading channel number\n");
        return ret;
    }

    ret=RMHApp_ReadUint32(app, &sourceNodeId);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading source node id\n");
        return ret;
    }

    ret=RMHApp_ReadUint32Hex(app, &destinationNodeMask);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading destination node mask\n");
        return ret;
    }

    ret=RMHApp_ReadACAType(app, &type);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading ACA type\n");
        return ret;
    }

    ret = api(app->rmh, channelNum, sourceNodeId, destinationNodeMask, type);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed in ACA request\n");
        return ret;
    }

    RMH_ACAStatus status;
    ret = RMH_ACA_GetStatus(app->rmh, &status);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("RMH_ACA_GetStatus: %s\n", RMH_ACAStatusToString(status));
    }

    int32_t txPower;
    ret = RMH_ACA_GetTotalRxPower(app->rmh, &txPower);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("RMH_ACA_GetTotalRxPower: %d\n", txPower);
    }

    RMH_PrintMsg("RMH_ACA_GetPowerProfile:\n");
    RMHApp__OUT_UINT8_ARRAY(app, RMH_ACA_GetPowerProfile);
    return RMH_SUCCESS;
 }

static
RMH_Result RMHApp__MoCA_RESET(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeMask, const uint32_t sleepTime)) {
    uint32_t nodeMask;
    uint32_t sleepTime;
    RMH_Result ret;

    ret=RMHApp_ReadUint32Hex(app, &nodeMask);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading node mask\n");
        return ret;
    }

    ret=RMHApp_ReadUint32(app, &sleepTime);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading sleep time\n");
        return ret;
    }

    ret = api(app->rmh, nodeMask, sleepTime);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed in reset request\n");
        return ret;
    }

    return ret;
}

static
RMH_Result RMHApp__OUT_RESET_REASON(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, RMH_MoCAResetReason* response)) {
    RMH_MoCAResetReason response;
    RMH_Result ret = api(app->rmh, &response);
    if (ret == RMH_SUCCESS) {
        RMH_PrintMsg("%s\n", RMH_MoCAResetReasonToString(response));
    }
    return ret;
}

static
void PrintModulation(RMHApp *app, uint32_t start, uint32_t end, RMH_SubcarrierProfile* array, size_t arraySize) {
    int i,j;
    int offset = start < end ? 1 : -1;
    for (i=start; i < arraySize;) {
        RMH_PrintMsg("[%3u-%3u] ", i, i+(31*offset));
        for (j=0; j<32 && i < arraySize; j++,i+=offset) {
            RMH_PrintMsg("%X", array[i]);
            if (i == end) {
                RMH_PrintMsg("\n");
                return;
            }
        }
        RMH_PrintMsg("\n");
    }
}

static
RMH_Result RMHApp__OUT_MODULATION(RMHApp *app, RMH_Result (*api)(const RMH_Handle handle, const uint32_t nodeId, const RMH_PERMode perMode, RMH_SubcarrierProfile* responseArray, const size_t responseArraySize, size_t* responseArrayUsed)) {
    uint32_t nodeId;
    RMH_SubcarrierProfile responseBuf[512];
    size_t responseBufUsed;
    RMH_Result ret;
    RMH_MoCAVersion response;
    RMH_PERMode perMode;

    ret=RMHApp_ReadUint32(app, &nodeId);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading node Id\n");
        return ret;
    }

    ret=RMHApp_ReadPERMode(app, &perMode);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed reading channel type\n");
        return ret;
    }

    ret = api(app->rmh, nodeId, perMode, responseBuf, sizeof(responseBuf)/sizeof(responseBuf[0]), &responseBufUsed);
    if (ret != RMH_SUCCESS) {
        return ret;
    }

    ret = RMH_RemoteNode_GetActiveMoCAVersion(app->rmh, nodeId, &response);
    if (ret != RMH_SUCCESS) {
        return ret;
    }
    switch (response) {
    case RMH_MOCA_VERSION_10:
    case RMH_MOCA_VERSION_11:
        PrintModulation(app, 127, 0, responseBuf, responseBufUsed);
        PrintModulation(app, 255, 128, responseBuf, responseBufUsed);
        break;
    case RMH_MOCA_VERSION_20:
        PrintModulation(app, 256, 511, responseBuf, responseBufUsed);
        PrintModulation(app, 0, 255, responseBuf, responseBufUsed);
        break;
    default:
        RMH_PrintErr("Unknown MoCA version.\n");
        break;
    }

    return ret;
}



/***********************************************************
 * Registration Functions
 ***********************************************************/
#define SET_LOCAL_API_HANDLER(HANDLER_FUNCTION, API_FUNCTION, ALIAS_STRING, DESCRIPTION_STR) \
    static RMH_API pRMH_API_##API_FUNCTION = { #API_FUNCTION, false, false, NULL, NULL, DESCRIPTION_STR, NULL, 0, NULL }; \
    if (app->local.apiListSize < RMH_MAX_NUM_APIS) app->local.apiList[app->local.apiListSize++]=&pRMH_API_##API_FUNCTION; \
    SET_API_HANDLER(HANDLER_FUNCTION, API_FUNCTION, ALIAS_STRING);

#define SET_API_HANDLER(HANDLER_FUNCTION, API_FUNCTION, ALIAS_STRING) { \
    while(0) HANDLER_FUNCTION(NULL, API_FUNCTION); \
    RMHApp_AddAPI(app, #API_FUNCTION, API_FUNCTION, HANDLER_FUNCTION, ALIAS_STRING); \
}

static
void RMHApp_AddAPI(RMHApp* app, const char* apiName, const void *apiFunc, const void *apiHandlerFunc, const char* apiAlias) {
    memset(&app->handledAPIs.apiList[app->handledAPIs.apiListSize], 0, sizeof(app->handledAPIs.apiList[app->handledAPIs.apiListSize]));
    app->handledAPIs.apiList[app->handledAPIs.apiListSize].apiName = apiName;
    app->handledAPIs.apiList[app->handledAPIs.apiListSize].apiAlias = (apiAlias && apiAlias[0] != '\0' ) ? apiAlias : NULL;
    app->handledAPIs.apiList[app->handledAPIs.apiListSize].apiFunc = apiFunc;
    app->handledAPIs.apiList[app->handledAPIs.apiListSize].apiHandlerFunc = apiHandlerFunc;
    app->handledAPIs.apiListSize++;
}

void RMHApp_RegisterAPIHandlers(RMHApp *app) {
    /* RMH APIs */
    /**************************************************************************************************************************************
     *                Handler Function              |                   API Name                          |     Alias (comma seperated)   *
     **************************************************************************************************************************************/
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetEnabled,                                    "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetEnabled,                                    "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetMoCALinkUp,                                 "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetLOF,                                        "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetLOF,                                        "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetScanLOFOnly,                                "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetScanLOFOnly,                                "");
    SET_API_HANDLER(RMHApp__OUT_MOCA_VERSION,                   RMH_Self_GetHighestSupportedMoCAVersion,                "");
    SET_API_HANDLER(RMHApp__OUT_STRING,                         RMH_Self_GetSoftwareVersion,                            "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetPreferredNCEnabled,                         "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetPreferredNCEnabled,                         "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetMaxPacketAggregation,                       "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetMaxPacketAggregation,                       "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetMaxFrameSize,                               "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetMaxFrameSize,                               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_HEX,                     RMH_Self_GetFrequencyMask,                              "");
    SET_API_HANDLER(RMHApp__IN_UINT32_HEX,                      RMH_Self_SetFrequencyMask,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetLowBandwidthLimit,                          "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetLowBandwidthLimit,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetMaxBitrate,                                 "");
    SET_API_HANDLER(RMHApp__IN_INT32,                           RMH_Self_SetTxPowerLimit,                               "");
    SET_API_HANDLER(RMHApp__OUT_INT32,                          RMH_Self_GetTxPowerLimit,                               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_ARRAY,                   RMH_Self_GetSupportedFrequencies,                       "");
    SET_API_HANDLER(RMHApp__HANDLE_ONLY,                        RMH_Self_RestoreDefaultSettings,                        "");
    SET_API_HANDLER(RMHApp__HANDLE_ONLY,                        RMH_Self_RestoreRDKDefaultSettings,                     "");
    SET_API_HANDLER(RMHApp__OUT_LINK_STATUS,                    RMH_Self_GetLinkStatus,                                 "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetQAM256Enabled,                              "");
    SET_API_HANDLER(RMHApp__OUT_BAND,                           RMH_Self_GetSupportedBand,                              "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetQAM256Enabled,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetQAM256TargetPhyRate,                        "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetQAM256TargetPhyRate,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetPrimaryChannelTargetPhyRate,                "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetPrimaryChannelTargetPhyRate,                "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetSecondaryChannelTargetPhyRate,              "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Self_SetSecondaryChannelTargetPhyRate,              "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetTurboEnabled,                               "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetTurboEnabled,                               "");
    SET_API_HANDLER(RMHApp__GET_TABOO,                          RMH_Self_GetTabooChannels,                              "");
    SET_API_HANDLER(RMHApp__SET_TABOO,                          RMH_Self_SetTabooChannels,                              "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetBondingEnabled,                             "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetBondingEnabled,                             "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Self_GetPrivacyEnabled,                             "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Self_SetPrivacyEnabled,                             "");
    SET_API_HANDLER(RMHApp__OUT_STRING,                         RMH_Self_GetPrivacyPassword,                            "");
    SET_API_HANDLER(RMHApp__IN_STRING,                          RMH_Self_SetPrivacyPassword,                            "");
    SET_API_HANDLER(RMHApp__OUT_STRING,                         RMH_Self_GetPrivacyMACManagementKey,                    "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Self_GetMaxAllocationElements,                      "");
    SET_API_HANDLER(RMHApp__IN_INT32,                           RMH_Self_SetPrimaryChannelOffset,                       "");
    SET_API_HANDLER(RMHApp__OUT_INT32,                          RMH_Self_GetPrimaryChannelOffset,                       "");
    SET_API_HANDLER(RMHApp__IN_INT32,                           RMH_Self_SetSecondaryChannelOffset,                     "");
    SET_API_HANDLER(RMHApp__OUT_INT32,                          RMH_Self_GetSecondaryChannelOffset,                     "");
    SET_API_HANDLER(RMHApp__OUT_RESET_REASON,                   RMH_Self_GetLastResetReason,                            "");

    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Interface_GetEnabled,                               "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Interface_SetEnabled,                               "");
    SET_API_HANDLER(RMHApp__OUT_STRING,                         RMH_Interface_GetName,                                  "");
    SET_API_HANDLER(RMHApp__OUT_MAC,                            RMH_Interface_GetMac,                                   "");
    SET_API_HANDLER(RMHApp__IN_MAC,                             RMH_Interface_SetMac,                                   "");

    SET_API_HANDLER(RMHApp__OUT_POWER_MODE,                     RMH_Power_GetMode,                                      "");
    SET_API_HANDLER(RMHApp__OUT_POWER_MODE,                     RMH_Power_GetStandbyMode,                               "");
    SET_API_HANDLER(RMHApp__IN_POWER_MODE,                      RMH_Power_SetStandbyMode,                               "");
    SET_API_HANDLER(RMHApp__OUT_POWER_MODE,                     RMH_Power_GetSupportedModes,                            "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Power_GetTxPowerControlEnabled,                     "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Power_SetTxPowerControlEnabled,                     "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Power_GetTxBeaconPowerReductionEnabled,             "");
    SET_API_HANDLER(RMHApp__IN_BOOL,                            RMH_Power_SetTxBeaconPowerReductionEnabled,             "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Power_GetTxBeaconPowerReduction,                    "");
    SET_API_HANDLER(RMHApp__IN_UINT32,                          RMH_Power_SetTxBeaconPowerReduction,                    "");

    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxTotalBytes,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxTotalBytes,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxTotalPackets,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxTotalPackets,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxUnicastPackets,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxUnicastPackets,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxBroadcastPackets,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxBroadcastPackets,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxMulticastPackets,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxMulticastPackets,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxReservationRequestPackets,               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxReservationRequestPackets,               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxMapPackets,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxMapPackets,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxLinkControlPackets,                      "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxLinkControlPackets,                      "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxUnknownProtocolPackets,                  "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxDroppedPackets,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxDroppedPackets,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxTotalErrors,                             "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxTotalErrors,                             "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxCRCErrors,                               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxTimeoutErrors,                           "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetAdmissionAttempts,                         "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetAdmissionSucceeded,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetAdmissionFailures,                         "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetAdmissionsDeniedAsNC,                      "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxBeacons,                                 "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxBeacons,                                 "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODELIST,                RMH_Stats_GetRxCorrectedErrors,                         "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODELIST,                RMH_Stats_GetRxUncorrectedErrors,                       "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetRxTotalAggregatedPackets,                  "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Stats_GetTxTotalAggregatedPackets,                  "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_ARRAY,                   RMH_Stats_GetRxPacketAggregation,                       "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_ARRAY,                   RMH_Stats_GetTxPacketAggregation,                       "");
    SET_API_HANDLER(RMHApp__HANDLE_ONLY,                        RMH_Stats_Reset,                                        "");

    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQOS_GetMaxIngressFlows,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQOS_GetMaxEgressFlows,                             "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQoS_GetNumIngressFlows,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQoS_GetNumEgressFlows,                             "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_PQoS_GetEgressBandwidth,                            "");
    SET_API_HANDLER(RMHApp__OUT_MAC_ARRAY,                      RMH_PQoS_GetIngressFlowIds,                             "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQoS_GetMaxEgressBandwidth,                         "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_PQoS_GetMinEgressBandwidth,                         "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetPeakDataRate,                           "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetBurstSize,                              "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetLeaseTime,                              "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetLeaseTimeRemaining,                     "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetFlowTag,                                "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetMaxLatency,                             "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetShortTermAvgRatio,                      "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetMaxRetry,                               "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetVLANTag,                                "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetFlowPer,                                "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetIngressClassificationRule,              "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetPacketSize,                             "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetTotalTxPackets,                         "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetDSCPMoCA,                               "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_UINT32,                  RMH_PQoSFlow_GetDFID,                                   "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_MAC,                     RMH_PQoSFlow_GetDestination,                            "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_MAC,                     RMH_PQoSFlow_GetIngressMac,                             "");
    SET_API_HANDLER(RMHApp__IN_MAC_OUT_MAC,                     RMH_PQoSFlow_GetEgressMac,                              "");

    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetNumNodes,                                "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetNodeId,                                  "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODELIST,                RMH_Network_GetNodeIds,                                 "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODELIST,                RMH_Network_GetRemoteNodeIds,                           "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODELIST,                RMH_Network_GetAssociatedIds,                           "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetNCNodeId,                                "");
    SET_API_HANDLER(RMHApp__OUT_MAC,                            RMH_Network_GetNCMac,                                   "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetBackupNCNodeId,                          "");
    SET_API_HANDLER(RMHApp__OUT_MOCA_VERSION,                   RMH_Network_GetMoCAVersion,                             "");
    SET_API_HANDLER(RMHApp__OUT_BOOL,                           RMH_Network_GetMixedMode,                               "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetLinkUptime,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetResetCount,                              "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetLinkDownCount,                           "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetRFChannelFreq,                           "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetPrimaryChannelFreq,                      "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetSecondaryChannelFreq,                    "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetTxMapPhyRate,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetTxBroadcastPhyRate,                      "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetTxGCDPhyRate,                            "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_Network_GetTxGcdPowerReduction,                     "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODEMESH,                RMH_Network_GetTxUnicastPhyRate,                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODEMESH,                RMH_Network_GetTxVLPER,                                 "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODEMESH,                RMH_Network_GetTxNPER,                                  "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODEMESH,                RMH_Network_GetBitLoadingInfo,                          "");
    SET_API_HANDLER(RMHApp__OUT_UINT32_NODEMESH,                RMH_Network_GetBondedConnections,                       "");
    SET_API_HANDLER(RMHApp__GET_TABOO,                          RMH_Network_GetTabooChannels,                           "");

    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetNodeIdFromAssociatedId,               "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetAssociatedIdFromNodeId,               "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_MAC,                  RMH_RemoteNode_GetMac,                                  "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_BOOL,                 RMH_RemoteNode_GetPreferredNC,                          "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_MOCA_VERSION,         RMH_RemoteNode_GetHighestSupportedMoCAVersion,          "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_MOCA_VERSION,         RMH_RemoteNode_GetActiveMoCAVersion,                    "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_BOOL,                 RMH_RemoteNode_GetQAM256Capable,                        "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_BOOL,                 RMH_RemoteNode_GetBondingCapable,                       "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetMaxPacketAggregation,                 "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetMaxFrameSize,                         "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxPackets,                            "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxUnicastPhyRate,                     "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxBroadcastPhyRate,                   "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_FLOAT,                RMH_RemoteNode_GetRxUnicastPower,                       "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_FLOAT,                RMH_RemoteNode_GetRxBroadcastPower,                     "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_FLOAT,                RMH_RemoteNode_GetRxMapPower,                           "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_FLOAT,                RMH_RemoteNode_GetRxSNR,                                "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxTotalErrors,                        "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxCorrectedErrors,                    "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetRxUnCorrectedErrors,                  "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_FLOAT,                RMH_RemoteNode_GetTxUnicastPower,                       "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetTxPowerReduction,                     "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetTxUnicastPhyRate,                     "");
    SET_API_HANDLER(RMHApp__IN_UINT32_OUT_UINT32,               RMH_RemoteNode_GetTxPackets,                            "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetRxUnicastSubcarrierModulation,        "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetTxUnicastSubcarrierModulation,        "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetSecondaryRxUnicastSubcarrierModulation, "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetSecondaryTxUnicastSubcarrierModulation, "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetRxBroadcastSubcarrierModulation,      "");
    SET_API_HANDLER(RMHApp__OUT_MODULATION,                     RMH_RemoteNode_GetTxBroadcastSubcarrierModulation,      "");
    SET_API_HANDLER(RMHApp__MoCA_RESET,                         RMH_RemoteNode_Reset,                                   "");

    SET_API_HANDLER(RMHApp__OUT_LOGLEVEL,                       RMH_Log_GetAPILevel,                                    "");
    SET_API_HANDLER(RMHApp__IN_LOGLEVEL,                        RMH_Log_SetAPILevel,                                    "");
    SET_API_HANDLER(RMHApp__OUT_LOGLEVEL,                       RMH_Log_GetDriverLevel,                                 "");
    SET_API_HANDLER(RMHApp__IN_LOGLEVEL,                        RMH_Log_SetDriverLevel,                                 "");
    SET_API_HANDLER(RMHApp__OUT_STRING,                         RMH_Log_GetDriverFilename,                              "");
    SET_API_HANDLER(RMHApp__IN_STRING,                          RMH_Log_SetDriverFilename,                              "");
    SET_API_HANDLER(RMHApp__PRINT_STATUS,                       RMH_Log_PrintStatus,                                    "status");
    SET_API_HANDLER(RMHApp__PRINT_STATUS,                       RMH_Log_PrintStats,                                     "stats");
    SET_API_HANDLER(RMHApp__PRINT_STATUS,                       RMH_Log_PrintFlows,                                     "flows");
    SET_API_HANDLER(RMHApp__PRINT_STATUS,                       RMH_Log_PrintModulation,                                "modulation");

    SET_API_HANDLER(RMHApp__REQUEST_ACA,                        RMH_ACA_Request,                                        "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_ACA_GetChannel,                                     "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_ACA_GetSourceNodeId,                                "");
    SET_API_HANDLER(RMHApp__OUT_UINT32,                         RMH_ACA_GetDestinationNodeMask,                         "");
    SET_API_HANDLER(RMHApp__OUT_ACA_TYPE,                       RMH_ACA_GetType,                                        "");
    SET_API_HANDLER(RMHApp__OUT_ACA_STATUS,                     RMH_ACA_GetStatus,                                      "");
    SET_API_HANDLER(RMHApp__OUT_INT32,                          RMH_ACA_GetTotalRxPower,                                "");
    SET_API_HANDLER(RMHApp__OUT_UINT8_ARRAY,                    RMH_ACA_GetPowerProfile,                                "");


    SET_API_HANDLER(RMHApp__IN_RMH_ENUM,                        RMH_ResultToString,                                     "");
    /* Local APIs */
    /*****************************************************************************************************************************************************************************************
     *                Handler Function              |                   API Name                          |     Alias (comma seperated)                      |                  Description  *
     *****************************************************************************************************************************************************************************************/
    SET_LOCAL_API_HANDLER(RMHApp__LOCAL_HANDLE_ONLY,            RMHApp_DriverLogForever,                                "log_forever,logforever",                       "Enabled debug logging to a dedicated file that does not rotate. This will collect the best logs however if left enabled for long periods of time you can run out of space.");
    SET_LOCAL_API_HANDLER(RMHApp__LOCAL_HANDLE_ONLY,            RMHApp_EnableDriverDebugLogging,                        "log",                                          "Enable debug logging to the default MoCA log location. This is safe to leave enabled for long periods of time as this default log should be rotated");
    SET_LOCAL_API_HANDLER(RMHApp__LOCAL_HANDLE_ONLY,            RMHApp_DisableDriverDebugLogging,                       "log_stop,logstop",                             "Disable MoCA debug logging");
    SET_LOCAL_API_HANDLER(RMHApp__LOCAL_HANDLE_ONLY,            RMHApp_Start,                                           "start",                                        "Shortcut to Enable MoCA");
    SET_LOCAL_API_HANDLER(RMHApp__LOCAL_HANDLE_ONLY,            RMHApp_Stop,                                            "stop",                                         "Shortcut to disable MoCA");
}
