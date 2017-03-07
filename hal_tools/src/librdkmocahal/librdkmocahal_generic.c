#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>


#include "rdk_moca_hal.h"


#define RMH_ENABLE_PRINT_ERR 1
#define RMH_ENABLE_PRINT_WRN 1
#define RMH_ENABLE_PRINT_LOG 1


static FILE *localLogToFile=NULL;
#define RMH_PrintErr(fmt, ...) if (RMH_ENABLE_PRINT_ERR) {fprintf(localLogToFile ? localLogToFile : stderr, "ERROR: " fmt "", ##__VA_ARGS__);}
#define RMH_PrintWrn(fmt, ...) if (RMH_ENABLE_PRINT_WRN) {fprintf(localLogToFile ? localLogToFile : stderr, "WARNING: " fmt "", ##__VA_ARGS__);}
#define RMH_PrintMsg(fmt, ...) if (RMH_ENABLE_PRINT_LOG) {fprintf(localLogToFile ? localLogToFile : stdout, fmt "", ##__VA_ARGS__);}

#define BRMH_RETURN_IF(expr, ret) { if (expr) { RMH_PrintErr("'" #expr "' is true!"); return ret; } }


#define RDK_FILE_PATH_VERSION               "/version.txt"
#define RDK_FILE_PATH_DEBUG_ENABLE          "/opt/rmh_start_enable_debug"
#define RDK_FILE_PATH_DEBUG_FOREVER_ENABLE  "/opt/rmh_start_enable_debug_forever"

RMH_Result RMH_Start(RMH_Handle handle) {
    bool started;

    /* Ensure MoCA is disabled on this device */
    BRMH_RETURN_IF(RMH_Self_GetEnabled(handle, &started) != RMH_SUCCESS, RMH_FAILURE);
    if (started) {
        RMH_PrintWrn("MoCA appears to already be started.\n");
        return RMH_INVALID_INTERNAL_STATE;
    }

    /* Return to a known good state */
    BRMH_RETURN_IF(RMH_Self_RestoreDefaultSettings(handle) != RMH_SUCCESS, RMH_FAILURE);

    /***** Setup debug ***********************/
    if (RMH_Log_SetLevel(handle, RMH_LOG_LEVEL_DEFAULT) != RMH_SUCCESS) {
        RMH_PrintWrn("Unable to set default log level!\n");
    }

    if (access(RDK_FILE_PATH_DEBUG_FOREVER_ENABLE, F_OK ) != -1) {
        char logFileName[1024];
        if (RMH_Log_CreateNewLogFile(handle, logFileName, sizeof(logFileName)) != RMH_SUCCESS) {
            RMH_PrintWrn("Unable to create dedicated log file!\n");
        }
        else if (RMH_Log_SetFilename(handle, logFileName) != RMH_SUCCESS) {
            RMH_PrintWrn("Unable to start logging in %s!\n", logFileName);
        }
        else if (RMH_Log_SetLevel(handle, RMH_LOG_LEVEL_DEBUG) != RMH_SUCCESS) {
            RMH_PrintWrn("Failed to set log level to RMH_LOG_LEVEL_DEBUG!\n");
        }
        else {
            RMH_PrintMsg("Setting debug logging enabled to file %s\n", logFileName);
        }
    }
    else if (access(RDK_FILE_PATH_DEBUG_ENABLE, F_OK ) != -1) {
        if (RMH_Log_SetLevel(handle, RMH_LOG_LEVEL_DEBUG) != RMH_SUCCESS) {
            RMH_PrintWrn("Unable to enable debug!\n");
        }
        else {
            RMH_PrintMsg("Setting debug logging enabled\n");
        }
    }
    /***** Done setup debug ***********************/


    /***** Setup device configuration ***********************/
#ifdef RMH_START_DEFAULT_SINGLE_CHANEL
    RMH_PrintMsg("Setting RMH_Self_SetLOF: %d\n", RMH_START_DEFAULT_SINGLE_CHANEL);
    BRMH_RETURN_IF(RMH_Self_SetScanLOFOnly(handle, true) != RMH_SUCCESS, RMH_FAILURE);
    BRMH_RETURN_IF(RMH_Self_SetLOF(handle, RMH_START_DEFAULT_SINGLE_CHANEL) != RMH_SUCCESS, RMH_FAILURE);
#endif

#ifdef RMH_START_DEFAULT_POWER_REDUCTION
    RMH_PrintMsg("Setting RMH_Power_SetTxBeaconPowerReduction: %d\n", RMH_START_DEFAULT_POWER_REDUCTION);
    BRMH_RETURN_IF(RMH_Power_SetTxBeaconPowerReductionEnabled(handle, true) != RMH_SUCCESS, RMH_FAILURE);
    BRMH_RETURN_IF(RMH_Power_SetTxBeaconPowerReduction(handle, RMH_START_DEFAULT_POWER_REDUCTION) != RMH_SUCCESS, RMH_FAILURE);
#endif

#ifdef RMH_START_DEFAULT_TABOO_START_CHANNEL
    RMH_PrintMsg("Setting RMH_Taboo_SetStartChannel: %d\n", RMH_START_DEFAULT_TABOO_START_CHANNEL);
    BRMH_RETURN_IF(RMH_Taboo_SetStartChannel(handle, RMH_START_DEFAULT_TABOO_START_CHANNEL) != RMH_SUCCESS, RMH_FAILURE);
#endif

#ifdef RMH_START_DEFAULT_TABOO_MASK
    RMH_PrintMsg("Setting RMH_Taboo_SetChannelMask: 0x%08x\n", RMH_START_DEFAULT_TABOO_MASK);
    BRMH_RETURN_IF(RMH_Taboo_SetChannelMask(handle, RMH_START_DEFAULT_TABOO_MASK) != RMH_SUCCESS, RMH_FAILURE);
#endif

#ifdef RMH_START_DEFAULT_PREFERRED_NC
    RMH_PrintMsg("Setting RMH_Self_SetPreferredNCEnabled: %s\n", RMH_START_DEFAULT_PREFERRED_NC ? "TRUE" : "FALSE");
    BRMH_RETURN_IF(RMH_Self_SetPreferredNCEnabled(handle, RMH_START_DEFAULT_PREFERRED_NC) != RMH_SUCCESS, RMH_FAILURE);
#endif

#if RMH_START_SET_MAC_FROM_PROC
    if (RMH_START_SET_MAC_FROM_PROC) {
        char ethName[18];
        unsigned char mac[6];
        char ifacePath[128];
        char macString[32];
        uint32_t macValsRead;
        FILE *file;
        BRMH_RETURN_IF(RMH_Interface_GetName(handle, ethName, sizeof(ethName)) != RMH_SUCCESS, RMH_FAILURE);
        sprintf(ifacePath, "/sys/class/net/%s/address", ethName);
		file = fopen( ifacePath, "r");
        BRMH_RETURN_IF(file == NULL, RMH_FAILURE);
        macValsRead=fscanf(file, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        BRMH_RETURN_IF(macValsRead != 6, RMH_FAILURE);
		fclose(file);
        RMH_PrintMsg("Setting RMH_Interface_SetMac[%s]: %s\n", ethName, RMH_MacToString(mac, macString, sizeof(macString)));
        BRMH_RETURN_IF(RMH_Interface_SetMac(handle, mac) != RMH_SUCCESS, RMH_FAILURE);
    }
#endif
    /***** Done setup device configuration ***********************/

    /* Enable MoCA */
    if (RMH_Self_SetEnabled(handle, true) != RMH_SUCCESS) {
        RMH_PrintErr("Failed calling RMH_Self_SetEnabled!\n");
        return RMH_FAILURE;
    }

    return RMH_SUCCESS;
}

RMH_Result RMH_Stop(RMH_Handle handle) {
    return RMH_Self_SetEnabled(handle, false);
}


RMH_Result RMH_Log_CreateNewLogFile(RMH_Handle handle, char* responseBuf, const size_t responseBufSize) {
    uint8_t mac[6];
    char line[1024];
    int i;
    struct tm* tm_info;
    struct timeval tv;
    FILE *logFile;
    FILE *verFile;
    int filenameLength;

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    if (RMH_Interface_GetMac(handle, &mac) != RMH_SUCCESS) {
        memset(mac, 0, sizeof(mac));
    }

    filenameLength=snprintf(responseBuf, responseBufSize, "/opt/rmh_moca_log_%s_MAC_%02X-%02X-%02X-%02X-%02X-%02X_TIME_%02d.%02d.%02d_%02d-%02d-%02d.log",
#ifdef MACHINE_NAME
        MACHINE_NAME,
#else
        "unknown",
#endif
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    BRMH_RETURN_IF(filenameLength<0, RMH_FAILURE);
    BRMH_RETURN_IF(filenameLength>=responseBufSize, RMH_INSUFFICIENT_SPACE);

    logFile=fopen(responseBuf, "w");
    if (logFile == NULL) {
        RMH_PrintErr("Unable to open %s for writing!\n", responseBuf);
        return RMH_FAILURE;
    }
    verFile=fopen(RDK_FILE_PATH_VERSION, "r");
    if (verFile == NULL) {
        RMH_PrintWrn("Unable to open %s for reading!. Cannot add version information\n", RDK_FILE_PATH_VERSION);
    }
    else {
        fprintf(logFile, "= Device Version information =================================================================================\n");
        for(i=0; i<10; i++) {
            if (fgets(line, sizeof(line), verFile) == NULL)
                break;
            fputs(line, logFile);
        }
        fprintf(logFile, "\n\n");
        fclose(verFile);
    }
    fclose(logFile);

    if (RMH_StatusWriteToFile(handle, responseBuf) != RMH_SUCCESS) {
        RMH_PrintWrn("Failed to dump the status summary in file -- %s\n", responseBuf);
    }

    return RMH_SUCCESS;
}


#define PRINT_STATUS_MACRO(api, type, apiFunc, fmt, ...) { \
    type; \
    ret = apiFunc; \
    if (ret == RMH_SUCCESS) { RMH_PrintMsg("%-50s: " fmt "\n", #api, ##__VA_ARGS__); } \
    else                    { RMH_PrintMsg("%-50s: %s\n", #api,  RMH_ResultToString(ret)); } \
}
#define RMH_Print_Status_BOOL(api)              PRINT_STATUS_MACRO(api, bool response,                          api(handle, &response),                         "%s", response ? "TRUE" : "FALSE");
#define RMH_Print_Status_BOOL_RN(api, i)        PRINT_STATUS_MACRO(api, bool response,                          api(handle, i, &response),                      "%s", response ? "TRUE" : "FALSE");
#define RMH_Print_Status_UINT32(api)            PRINT_STATUS_MACRO(api, uint32_t response,                      api(handle, &response),                         "%u", response);
#define RMH_Print_Status_UINT32_RN(api, i)      PRINT_STATUS_MACRO(api, uint32_t response,                      api(handle, i, &response),                      "%u", response);
#define RMH_Print_Status_UINT32_HEX(api)        PRINT_STATUS_MACRO(api, uint32_t response,                      api(handle, &response),                         "0x%08x", response);
#define RMH_Print_Status_UINT32_HEX_RN(api, i)  PRINT_STATUS_MACRO(api, uint32_t response,                      api(handle, i, &response),                      "0x%08x", response);
#define RMH_Print_Status_UPTIME(api)            PRINT_STATUS_MACRO(api, uint32_t response,                      api(handle, &response),                         "%02uh:%02um:%02us", response/3600, (response%3600)/60, response%60);
#define RMH_Print_Status_STRING(api)            PRINT_STATUS_MACRO(api, char response[256],                     api(handle, response, sizeof(response)),        "%s", response);
#define RMH_Print_Status_STRING_RN(api, i)      PRINT_STATUS_MACRO(api, char response[256],                     api(handle, i, response, sizeof(response)),     "%s", response);
#define RMH_Print_Status_MAC(api)               PRINT_STATUS_MACRO(api, uint8_t response[6];char macStr[24],    api(handle, &response),                         "%s", RMH_MacToString(response, macStr, sizeof(macStr)));
#define RMH_Print_Status_MAC_RN(api, i)         PRINT_STATUS_MACRO(api, uint8_t response[6];char macStr[24],    api(handle, i, &response),                      "%s", RMH_MacToString(response, macStr, sizeof(macStr)));
#define RMH_Print_Status_MoCAVersion(api)       PRINT_STATUS_MACRO(api, RMH_MoCAVersion response,               api(handle, &response),                         "%s", RMH_MoCAVersionToString(response));
#define RMH_Print_Status_MoCAVersion_RN(api, i) PRINT_STATUS_MACRO(api, RMH_MoCAVersion response,               api(handle, i, &response),                      "%s", RMH_MoCAVersionToString(response));
#define RMH_Print_Status_PowerMode(api)         PRINT_STATUS_MACRO(api, RMH_PowerMode response,                 api(handle, &response),                         "%s", RMH_PowerModeToString(response));
#define RMH_Print_Status_PowerMode_RN(api, i)   PRINT_STATUS_MACRO(api, RMH_PowerMode response,                 api(handle, i, &response),                      "%s", RMH_PowerModeToString(response));
#define RMH_Print_Status_FLOAT(api)             PRINT_STATUS_MACRO(api, float response,                         api(handle, &response),                         "%.02f", response);
#define RMH_Print_Status_FLOAT_RN(api, i)       PRINT_STATUS_MACRO(api, float response,                         api(handle,i, &response),                       "%.02f", response);
#define RMH_Print_Status_LOG_LEVEL(api)         PRINT_STATUS_MACRO(api, RMH_LogLevel response,                  api(handle, &response),                         "%s", RMH_LogLevelToString(response));

/* Do this one seperate so we can use lsResponse */
#define RMH_Print_Status_LinkStatus(api) { \
    ret = api(handle, &lsResponse); \
    if (ret == RMH_SUCCESS) { RMH_PrintMsg("%-50s: %s\n", #api, RMH_LinkStatusToString(lsResponse)); } \
    else                    { RMH_PrintMsg("%-50s: %s\n", #api, RMH_ResultToString(ret)); } \
}

#define RMH_Print_Status_NODE_LIST(api) _RMH_Print_Status_NODE_LIST(handle, #api, api)
static RMH_Result _RMH_Print_Status_NODE_LIST(RMH_Handle handle, const char *apiName, RMH_Result (*api)(RMH_Handle handle, RMH_NodeList_Uint32_t* response)) {
    RMH_NodeList_Uint32_t response;
    RMH_Result ret;
    int i;

    RMH_PrintMsg("\n- %s ---------\n", apiName);
    ret = api(handle, &response);
    if (ret == RMH_SUCCESS) {
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) { RMH_PrintMsg("    Node %02u: %u\n", i, response.nodeValue[i]); }
        }
    }
    else {
        RMH_PrintMsg("    %s\n", RMH_ResultToString(ret));
    }

    return RMH_SUCCESS;
}


#define RMH_Print_Status_NODE_MESH(api) _RMH_Print_Status_NODE_MESH(handle, #api, api)
static RMH_Result _RMH_Print_Status_NODE_MESH(RMH_Handle handle, const char *apiName, RMH_Result (*api)(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response)) {
    RMH_NodeMesh_Uint32_t response;
    RMH_Result ret;
    int i, j;

    RMH_PrintMsg("\n- %s ---------\n", apiName);
    ret = api(handle, &response);
    if (ret == RMH_SUCCESS) {
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                RMH_NodeList_Uint32_t *nl = &response.nodeValue[i];
                RMH_PrintMsg("    Node %02u:\n", i);
                for (j = 0; j < RMH_MAX_MOCA_NODES; j++) {
                    if (i != j && response.nodePresent[j]) {
                        RMH_PrintMsg("        Node %02u: %u\n", j, nl->nodeValue[j]);
                    }
                }
                RMH_PrintMsg("    \n");
            }
        }
    }
    else {
        RMH_PrintMsg("    %s\n", RMH_ResultToString(ret));
    }
    return RMH_SUCCESS;
}

RMH_Result RMH_Status(RMH_Handle handle) {
    RMH_Result ret;
    bool enabled;
    int i;

    RMH_PrintMsg("= RMH Local Device Status =================================================================================\n");
    RMH_Print_Status_BOOL(RMH_Self_GetEnabled);
    RMH_Print_Status_STRING(RMH_Interface_GetName);
    RMH_Print_Status_MAC(RMH_Interface_GetMac);
    RMH_Print_Status_STRING(RMH_Self_GetSoftwareVersion);
    RMH_Print_Status_MoCAVersion(RMH_Self_GetHighestSupportedMoCAVersion);
    RMH_Print_Status_BOOL(RMH_Self_GetPreferredNCEnabled);
    RMH_Print_Status_UINT32(RMH_Self_GetLOF);
    RMH_Print_Status_BOOL(RMH_Power_GetTxPowerControlEnabled);
    RMH_Print_Status_BOOL(RMH_Power_GetTxBeaconPowerReductionEnabled);
    RMH_Print_Status_UINT32(RMH_Power_GetTxBeaconPowerReduction);
    RMH_Print_Status_UINT32(RMH_Taboo_GetStartChannel);
    RMH_Print_Status_UINT32_HEX(RMH_Taboo_GetChannelMask);
    RMH_Print_Status_UINT32_HEX(RMH_Self_GetFreqMask);
    RMH_Print_Status_BOOL(RMH_QAM256_GetEnabled);
    RMH_Print_Status_BOOL(RMH_Turbo_GetEnabled);
    RMH_Print_Status_BOOL(RMH_Bonding_GetEnabled);
    RMH_Print_Status_BOOL(RMH_Privacy_GetEnabled);
    RMH_Print_Status_STRING(RMH_Privacy_GetPassword);
    RMH_Print_Status_BOOL(RMH_Interface_GetEnabled);
    RMH_Print_Status_LOG_LEVEL(RMH_Log_GetLevel);
    RMH_Print_Status_STRING(RMH_Log_GetFilename);

    ret=RMH_Self_GetEnabled(handle, &enabled);
    if (ret == RMH_SUCCESS && enabled) {
        RMH_LinkStatus lsResponse;
        RMH_NodeList_Uint32_t nodes;

        RMH_Print_Status_PowerMode(RMH_Power_GetMode);
        RMH_PrintMsg("\n= RMH Network Status ===================================================================================\n");
        RMH_Print_Status_LinkStatus(RMH_Network_GetLinkStatus);
        if (lsResponse == RMH_INTERFACE_UP) {
            RMH_Print_Status_UINT32(RMH_Network_GetNumNodes);
            RMH_Print_Status_UINT32(RMH_Self_GetNodeId);
            RMH_Print_Status_UINT32(RMH_Network_GetNCNodeId);
            RMH_Print_Status_MAC(RMH_Network_GetNCMac);
            RMH_Print_Status_UINT32(RMH_Network_GetBackupNCNodeId);
            RMH_Print_Status_UPTIME(RMH_Network_GetLinkUptime);
            RMH_Print_Status_MoCAVersion(RMH_Network_GetMoCAVersion);
            RMH_Print_Status_BOOL(RMH_Network_GetMixedMode);
            RMH_Print_Status_UINT32(RMH_Network_GetRFChannelFreq);
            RMH_Print_Status_UINT32(RMH_Stats_GetTxTotalPackets);
            RMH_Print_Status_UINT32(RMH_Stats_GetRxTotalPackets);
            RMH_Print_Status_UINT32(RMH_Stats_GetTxTotalErrors);
            RMH_Print_Status_UINT32(RMH_Stats_GetRxTotalErrors);
            RMH_Print_Status_UINT32(RMH_Stats_GetTxDroppedPackets);
            RMH_Print_Status_UINT32(RMH_Stats_GetRxDroppedPackets);
            ret=RMH_Network_GetRemoteNodeId(handle, &nodes);
            if (ret == RMH_SUCCESS) {
                for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                    if (nodes.nodePresent[i]) {
                        RMH_PrintMsg("\n= RMH Remote Node ID %02u =========================================================================\n", i);
                        RMH_Print_Status_MAC_RN(RMH_RemoteNode_GetMac, i);
                        RMH_Print_Status_MoCAVersion_RN(RMH_RemoteNode_GetHighestSupportedMoCAVersion, i);
                        RMH_Print_Status_BOOL_RN(RMH_RemoteNode_GetPreferredNC, i);
                        RMH_Print_Status_UINT32_RN(RMH_RemoteNodeRx_GetPackets, i);
                        RMH_Print_Status_UINT32_RN(RMH_RemoteNodeTx_GetPackets, i);
                        RMH_Print_Status_UINT32_RN(RMH_RemoteNodeTx_GetUnicastPhyRate, i);
                        RMH_Print_Status_UINT32_RN(RMH_RemoteNodeTx_GetPowerReduction, i);
                        RMH_Print_Status_FLOAT_RN(RMH_RemoteNodeTx_GetUnicastPower, i);
                        RMH_Print_Status_UINT32_RN(RMH_RemoteNodeRx_GetTotalErrors, i);
                        RMH_Print_Status_FLOAT_RN(RMH_RemoteNodeRx_GetUnicastPower, i);
                        RMH_Print_Status_FLOAT_RN(RMH_RemoteNodeRx_GetSNR, i);
                    }
                }
            }
            RMH_Print_Status_NODE_MESH(RMH_NetworkMesh_GetTxUnicastPhyRate);
        }
    }
    else {
        RMH_PrintMsg("\n\n");
        RMH_PrintWrn("*** MoCA not connected! ***\n");
    }
    RMH_PrintMsg("============================================================================================================\n\n");
    return RMH_SUCCESS;
}

RMH_Result RMH_StatusWriteToFile(RMH_Handle handle, const char* filename) {
    RMH_Result ret;
    localLogToFile=fopen(filename, "a");
    if (localLogToFile == NULL) {
        RMH_PrintErr("Failed to open '%s' for writing!\n", filename);
        return RMH_FAILURE;
    }

    ret=RMH_Status(handle);
    fclose(localLogToFile);
    localLogToFile=NULL;
    return ret;
}

