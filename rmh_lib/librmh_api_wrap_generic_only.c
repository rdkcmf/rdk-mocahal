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

#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <ctype.h>
#include "librmh.h"
#include "rdk_moca_hal.h"

#define RDK_FILE_PATH_VERSION               "/version.txt"
#define RDK_FILE_PATH_DEBUG_ENABLE          "/opt/rmh_start_enable_debug"
#define RDK_FILE_PATH_DEBUG_FOREVER_ENABLE  "/opt/rmh_start_enable_debug_forever"
#define LOCAL_MODULATION_PRINT_LINE_SIZE 256

static
RMH_Result pRMH_IOCTL_Get(const RMH_Handle handle, const char* name, struct ifreq *ifrq) {
    int fd = -1, rc = 0;
    RMH_Result ret=RMH_SUCCESS;

    BRMH_RETURN_IF(!name, RMH_INVALID_PARAM)
    BRMH_RETURN_IF((fd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0, RMH_FAILURE);

    strncpy(ifrq->ifr_name, name, sizeof(ifrq->ifr_name));
    ifrq->ifr_name[ sizeof(ifrq->ifr_name)-1 ] = 0;
    rc = ioctl(fd, SIOCGIFFLAGS, ifrq);
    if (rc < 0)	{
        RMH_PrintErr("ioctl SIOCGIFFLAGS returned %d!", rc);
        return RMH_FAILURE;
    }

    close(fd);
    return ret;
}

static
RMH_Result pRMH_IOCTL_Set(const RMH_Handle handle, struct ifreq *ifrq) {
    int fd = -1, rc = 0;
    RMH_Result ret=RMH_SUCCESS;

    BRMH_RETURN_IF((fd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0, RMH_FAILURE);
    rc = ioctl(fd, SIOCSIFFLAGS, ifrq);
    if (rc < 0)	{
        RMH_PrintErr("ioctl SIOCSIFFLAGS returned %d!", rc);
        return RMH_FAILURE;
    }

    close(fd);
    return ret;
}

static
RMH_Result pRMH_FindTagList(const RMH_Handle handle, const char * tag, RMH_APIList **apiList) {
    int i;

    /*Skip whitespace */
    while( tag[0] != '\0' && tag[0] == ' ' ) tag++;

    if (strlen(tag) > 0) {
        for(i=0; i != hRMHGeneric_APITags.tagListSize; i++) {
            if (strcasecmp(tag, hRMHGeneric_APITags.tagList[i].apiListName) == 0) {
                *apiList=&hRMHGeneric_APITags.tagList[i];
                return RMH_SUCCESS;
            }
        }
        if (hRMHGeneric_APITags.tagListSize < RMH_MAX_NUM_TAGS) {
            *apiList=&hRMHGeneric_APITags.tagList[hRMHGeneric_APITags.tagListSize++];
            (*apiList)->apiListSize=0;
            strncpy((*apiList)->apiListName, tag, sizeof((*apiList)->apiListName));
            (*apiList)->apiListName[0]=toupper((*apiList)->apiListName[0]);
            RMH_PrintDbg("New tag '%s' add. Total tags:%d\n", (*apiList)->apiListName, hRMHGeneric_APITags.tagListSize);
            return RMH_SUCCESS;
        }
        RMH_PrintWrn("Unable to find an existing tag '%s' and no more new tags are available. This will be dropped.\n", tag);
    }
    return RMH_FAILURE;
}

static
int pRMH_TagCompare(const void* a, const void* b) {
    const RMH_APIList _a=* (RMH_APIList const *)a;
    const RMH_APIList _b=* (RMH_APIList const *)b;
    return strcasecmp(_a.apiListName, _b.apiListName);
}


RMH_Result GENERIC_IMPL__RMH_Self_SetEnabledRDK(const RMH_Handle handle) {
    bool started;

    /* Ensure MoCA is disabled on this device */
    BRMH_RETURN_IF_FAILED(RMH_Self_GetEnabled(handle, &started));
    if (started) {
        RMH_PrintErr("MoCA is already enabled\n");
        return RMH_INVALID_INTERNAL_STATE;
    }

    /* Return to a known good state */
    BRMH_RETURN_IF_FAILED(RMH_Self_RestoreDefaultSettings(handle));

    /***** Setup debug ***********************/
    if (RMH_Log_SetDriverLevel(handle, RMH_LOG_DEFAULT) != RMH_SUCCESS) {
        RMH_PrintWrn("Unable to set default log level!\n");
    }

    if (access(RDK_FILE_PATH_DEBUG_FOREVER_ENABLE, F_OK ) != -1) {
        char logFileName[1024];
        if (RMH_Log_CreateDriverFile(handle, logFileName, sizeof(logFileName)) != RMH_SUCCESS) {
            RMH_PrintWrn("Unable to create dedicated log file!\n");
        }
        else if (RMH_Log_SetDriverFilename(handle, logFileName) != RMH_SUCCESS) {
            RMH_PrintWrn("Unable to start logging in %s!\n", logFileName);
        }
        else if (RMH_Log_SetDriverLevel(handle, RMH_LOG_DEFAULT | RMH_LOG_DEBUG) != RMH_SUCCESS) {
            RMH_PrintWrn("Failed to enable RMH_LOG_DEBUG!\n");
        }
        else {
            RMH_PrintMsg("Setting debug logging enabled to file %s\n", logFileName);
        }
    }
    else if (access(RDK_FILE_PATH_DEBUG_ENABLE, F_OK ) != -1) {
        if (RMH_Log_SetDriverLevel(handle, RMH_LOG_DEFAULT | RMH_LOG_DEBUG) != RMH_SUCCESS) {
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
    BRMH_RETURN_IF_FAILED(RMH_Self_SetScanLOFOnly(handle, true));
    BRMH_RETURN_IF_FAILED(RMH_Self_SetLOF(handle, RMH_START_DEFAULT_SINGLE_CHANEL));
#endif

#ifdef RMH_START_DEFAULT_POWER_REDUCTION
    RMH_PrintMsg("Setting RMH_Power_SetTxBeaconPowerReduction: %d\n", RMH_START_DEFAULT_POWER_REDUCTION);
    BRMH_RETURN_IF_FAILED(RMH_Power_SetTxBeaconPowerReductionEnabled(handle, true));
    BRMH_RETURN_IF_FAILED(RMH_Power_SetTxBeaconPowerReduction(handle, RMH_START_DEFAULT_POWER_REDUCTION));
#endif

#if defined RMH_START_DEFAULT_TABOO_START_CHANNEL && defined RMH_START_DEFAULT_TABOO_MASK
    RMH_PrintMsg("Setting RMH_Self_SetTabooChannels: Start:%d Channel Mask:0x%08x\n", RMH_START_DEFAULT_TABOO_START_CHANNEL, RMH_START_DEFAULT_TABOO_MASK);
    BRMH_RETURN_IF_FAILED(RMH_Self_SetTabooChannels(handle, RMH_START_DEFAULT_TABOO_START_CHANNEL, RMH_START_DEFAULT_TABOO_MASK));
#endif

#ifdef RMH_START_DEFAULT_TABOO_MASK
#endif

#ifdef RMH_START_DEFAULT_PREFERRED_NC
    RMH_PrintMsg("Setting RMH_Self_SetPreferredNCEnabled: %s\n", RMH_START_DEFAULT_PREFERRED_NC ? "TRUE" : "FALSE");
    BRMH_RETURN_IF_FAILED(RMH_Self_SetPreferredNCEnabled(handle, RMH_START_DEFAULT_PREFERRED_NC));
#endif

#if RMH_START_SET_MAC_FROM_PROC
    if (RMH_START_SET_MAC_FROM_PROC) {
        char ethName[18];
        unsigned char mac[6];
        char ifacePath[128];
        char macString[32];
        uint32_t macValsRead;
        FILE *file;
        BRMH_RETURN_IF_FAILED(RMH_Interface_GetName(handle, ethName, sizeof(ethName)));
        sprintf(ifacePath, "/sys/class/net/%s/address", ethName);
		file = fopen( ifacePath, "r");
        BRMH_RETURN_IF(file == NULL, RMH_FAILURE);
        macValsRead=fscanf(file, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        BRMH_RETURN_IF(macValsRead != 6, RMH_FAILURE);
		fclose(file);
        RMH_PrintMsg("Setting RMH_Interface_SetMac[%s]: %s\n", ethName, RMH_MacToString(mac, macString, sizeof(macString)));
        BRMH_RETURN_IF_FAILED(RMH_Interface_SetMac(handle, mac));
    }
#endif

    /* BCOM-2548: Force MoCA to remain active when the box goes to standby. This will cost some power savings but is needed
                  to work around an interop issue on the Cisco Xb3. When that device is the NC of a mixed mode network it 
                  seems to have issues with a node going to standby */
    RMH_Power_SetStandbyMode(handle, RMH_POWER_MODE_M0_ACTIVE);

    /***** Done setup device configuration ***********************/

    /* Enable MoCA */
    if (RMH_Self_SetEnabled(handle, true) != RMH_SUCCESS) {
        RMH_PrintErr("Failed calling RMH_Self_SetEnabled!\n");
        return RMH_FAILURE;
    }

    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_Log_CreateDriverFile(const RMH_Handle handle, char* responseBuf, const size_t responseBufSize) {
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

    if (RMH_Log_PrintStatus(handle, responseBuf) != RMH_SUCCESS) {
        RMH_PrintWrn("Failed to dump the status summary in file -- %s\n", responseBuf);
    }

    return RMH_SUCCESS;
}

/* Return the maximum egress bandwith from all nodes on the network */
RMH_Result GENERIC_IMPL__RMH_PQoS_GetMaxEgressBandwidth(const RMH_Handle handle, uint32_t* response) {
    RMH_NodeList_Uint32_t remoteNodes;
    uint32_t nodeResponse;
    uint32_t nodeId;
    RMH_Result ret = RMH_FAILURE;

    *response=0;
    BRMH_RETURN_IF_FAILED(RMH_Network_GetRemoteNodeIds(handle, &remoteNodes));
    for (nodeId = 0; nodeId < RMH_MAX_MOCA_NODES; nodeId++) {
        if (remoteNodes.nodePresent[nodeId]) {
            if (RMH_PQoS_GetEgressBandwidth(handle, nodeId, &nodeResponse) == RMH_SUCCESS) {
                if (nodeResponse > *response) {
                    *response = nodeResponse;
                    ret=RMH_SUCCESS;
                }
            }
            else {
                RMH_PrintWrn("Failed to get info for node Id %u!", nodeId);
            }
        }
    }

    return ret;
}

/* Return the minumum egress bandwith from all nodes on the network */
RMH_Result GENERIC_IMPL__RMH_PQoS_GetMinEgressBandwidth(const RMH_Handle handle, uint32_t* response) {
    RMH_NodeList_Uint32_t remoteNodes;
    uint32_t nodeResponse;
    uint32_t nodeId;
    RMH_Result ret = RMH_FAILURE;

    *response=0xFFFFFFFF;
    BRMH_RETURN_IF_FAILED(RMH_Network_GetRemoteNodeIds(handle, &remoteNodes));
    for (nodeId = 0; nodeId < RMH_MAX_MOCA_NODES; nodeId++) {
        if (remoteNodes.nodePresent[nodeId]) {
            if (RMH_PQoS_GetEgressBandwidth(handle, nodeId, &nodeResponse) == RMH_SUCCESS) {
                if (nodeResponse < *response) {
                    *response = nodeResponse;
                    ret=RMH_SUCCESS;
                }
            }
            else {
                RMH_PrintWrn("Failed to get info for node Id %u!\n", nodeId);
            }
        }
    }

    return ret;
}

RMH_Result GENERIC_IMPL__RMH_Interface_GetEnabled(const RMH_Handle handle, bool *response) {
    char ethName[18];
    struct ifreq ifrq;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(response==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF_FAILED(RMH_Interface_GetName(handle, ethName, sizeof(ethName)));
    BRMH_RETURN_IF(pRMH_IOCTL_Get(handle, ethName, &ifrq) != RMH_SUCCESS, RMH_FAILURE);
    if ((ifrq.ifr_flags & IFF_UP) &&
        (ifrq.ifr_flags & IFF_RUNNING)) {
        *response = true;
    }
    else {
        *response = false;
    }
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_Interface_SetEnabled(const RMH_Handle handle, const bool value) {
    char ethName[18];
    struct ifreq ifrq;
    bool selectedValue;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF_FAILED(RMH_Interface_GetName(handle, ethName, sizeof(ethName)));
    BRMH_RETURN_IF(pRMH_IOCTL_Get(handle, ethName, &ifrq) != RMH_SUCCESS, RMH_FAILURE);
    if (value)
        ifrq.ifr_flags |= IFF_UP|IFF_RUNNING;
    else
        ifrq.ifr_flags &= ~(IFF_UP|IFF_RUNNING);

    BRMH_RETURN_IF(pRMH_IOCTL_Set(handle, &ifrq) != RMH_SUCCESS, RMH_FAILURE);

    RMH_Interface_GetEnabled(handle, &selectedValue);
    return (selectedValue==value) ? RMH_SUCCESS : RMH_FAILURE;
}

RMH_Result GENERIC_IMPL__RMH_Self_GetLinkStatus(const RMH_Handle handle, RMH_LinkStatus* response) {
    bool linkUp;
    bool selfEnabled;
    bool ifEnabled;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(response==NULL, RMH_INVALID_PARAM);

    BRMH_RETURN_IF_FAILED(RMH_Self_GetEnabled(handle, &selfEnabled));
    if (!selfEnabled) {
        *response=RMH_LINK_STATUS_DISABLED;
        return RMH_SUCCESS;
    }

    BRMH_RETURN_IF_FAILED(RMH_Self_GetMoCALinkUp(handle, &linkUp));
    if (!linkUp) {
        *response=RMH_LINK_STATUS_NO_LINK;
        return RMH_SUCCESS;
    }

    BRMH_RETURN_IF_FAILED(RMH_Interface_GetEnabled(handle, &ifEnabled));
    if (!ifEnabled) {
        *response=RMH_LINK_STATUS_INTERFACE_DOWN;
        return RMH_SUCCESS;
    }

    *response=RMH_LINK_STATUS_UP;
    return RMH_SUCCESS;
}

static inline
uint32_t PrintModulationToString(char *outBuf, uint32_t outBufSize, const bool firstPrint, const RMH_SubcarrierProfile* array, const size_t arraySize, const uint32_t end, int32_t *start, bool *done) {
    int32_t j;
    int32_t pos=*start;
    int32_t  offset = pos < end ? 1 : -1;
    int32_t outBufRemaining=outBufSize;
    bool disablePrint=false;
    int charsToBeWritten;

    if (firstPrint) {
        charsToBeWritten = snprintf(outBuf, outBufRemaining, "    |  [%3u-%3u]  |  ", pos, pos+(31*offset));
        /* Update the remaining space in the buffer */
        if (charsToBeWritten < 0 || charsToBeWritten >= outBufRemaining) charsToBeWritten=outBufRemaining;
        outBuf += charsToBeWritten;
        outBufRemaining -= charsToBeWritten;
    }

    for (j=0; j<32; j++) {
        /* If the index is out of bounds or we've reached the end, stop printing */
        if (*done || pos >= arraySize || pos < 0) disablePrint=true;

        /* If printing is disabled still print a space to keep columns in order */
        charsToBeWritten = disablePrint ? snprintf(outBuf, outBufRemaining, " ") :
                                          snprintf(outBuf, outBufRemaining, "%X", array[pos]);

        /* Update the remaining space in the buffer */
        if (charsToBeWritten < 0 || charsToBeWritten >= outBufRemaining) charsToBeWritten=outBufRemaining;
        outBuf += charsToBeWritten;
        outBufRemaining -= charsToBeWritten;

        /* Check if we've reached the 'end'. This variable is enclusive so check after the print. */
        if (pos == end) *done=true;
        if (!*done) pos+=offset;
    }

    charsToBeWritten = snprintf(outBuf, outBufRemaining, "  |  ");
    /* Update the remaining space in the buffer */
    if (charsToBeWritten < 0 || charsToBeWritten >= outBufRemaining) charsToBeWritten=outBufRemaining;
    outBuf += charsToBeWritten;
    outBufRemaining -= charsToBeWritten;

    *start=pos;
    return outBufSize - outBufRemaining;
}

static
RMH_Result RMH_Print_MODULATION(const RMH_Handle handle, const uint32_t start, const uint32_t end,
                                                            const char*h0, const RMH_SubcarrierProfile* p0, const size_t p0Size,
                                                            const char*h1, const RMH_SubcarrierProfile* p1, const size_t p1Size,
                                                            const char*h2, const RMH_SubcarrierProfile* p2, const size_t p2Size,
                                                            const char*h3, const RMH_SubcarrierProfile* p3, const size_t p3Size) {
    char line[LOCAL_MODULATION_PRINT_LINE_SIZE];
    const char *lineEnd=line + LOCAL_MODULATION_PRINT_LINE_SIZE;

    #define MAX_COLS 4
    int32_t i[MAX_COLS];
    bool done[MAX_COLS];
    bool valid[MAX_COLS];
    const char* printHeader[MAX_COLS];
    uint32_t printHeaderLen[MAX_COLS];
    bool complete=false;
    uint32_t headerLength = 0;
    int j;

    valid[0] = (p0 != NULL && p0Size >0);
    valid[1] = (p1 != NULL && p1Size >0);
    valid[2] = (p2 != NULL && p2Size >0);
    valid[3] = (p3 != NULL && p3Size >0);
    printHeader[0] = (h0 != NULL && valid[0]) ? h0 : "";
    printHeader[1] = (h1 != NULL && valid[1]) ? h1 : "";
    printHeader[2] = (h2 != NULL && valid[2]) ? h2 : "";
    printHeader[3] = (h3 != NULL && valid[3]) ? h3 : "";
    printHeaderLen[0] = strlen(printHeader[0]);
    printHeaderLen[1] = strlen(printHeader[1]);
    printHeaderLen[2] = strlen(printHeader[2]);
    printHeaderLen[3] = strlen(printHeader[3]);

    for(j=0; j != MAX_COLS; j++) {
        i[j]=start;
        done[j]=false;
        headerLength += printHeaderLen[j];
    }

    if (headerLength > 0) {
        RMH_PrintMsg("    | Subcarrier  | ");
        for(j=0; j != MAX_COLS; j++) {
            if (printHeaderLen[j] > 0) {
                RMH_PrintMsg("%-34s | ", printHeader[j]);
            }
        }

        RMH_PrintMsg("\n     -------------|%s%s%s%s\n",
                                (printHeaderLen[0] > 0) ? "------------------------------------|" : "",
                                (printHeaderLen[1] > 0) ? "------------------------------------|" : "",
                                (printHeaderLen[2] > 0) ? "------------------------------------|" : "",
                                (printHeaderLen[3] > 0) ? "------------------------------------|" : "");
    }

    while(!complete) {
        char *linePos=line;
        uint32_t lineRemaining=LOCAL_MODULATION_PRINT_LINE_SIZE-1; /* -1 for NULL terminator */
        complete=true;

        if (valid[0]) {
            linePos+=PrintModulationToString(linePos, lineRemaining, true,  p0, p0Size, end, &i[0], &done[0]);
            lineRemaining=linePos>lineEnd ? 0 : lineEnd-linePos;
            complete&=done[0];
        }

        if (valid[1]) {
            linePos+=PrintModulationToString(linePos, lineRemaining, false,  p1, p1Size, end, &i[1], &done[1]);
            lineRemaining=linePos>lineEnd ? 0 : lineEnd-linePos;
            complete&=done[1];
        }

        if (valid[2]) {
            linePos+=PrintModulationToString(linePos, lineRemaining, false,  p2, p2Size, end, &i[2], &done[2]);
            lineRemaining=linePos>lineEnd ? 0 : lineEnd-linePos;
            complete&=done[2];
        }

        if (valid[3]) {
            linePos+=PrintModulationToString(linePos, lineRemaining, false,  p3, p3Size, end, &i[3], &done[3]);
            lineRemaining=linePos>lineEnd ? 0 : lineEnd-linePos;
            complete&=done[3];
        }

        RMH_PrintMsg("%s\n", line);
    }
    return RMH_SUCCESS;
}

static
RMH_Result RMHApp__OUT_UINT32_NODELIST(const RMH_Handle handle, RMH_Result (*api)(const RMH_Handle handle, RMH_NodeList_Uint32_t* response)) {
    RMH_NodeList_Uint32_t response;

    RMH_Result ret = api(handle, &response);
    if (ret == RMH_SUCCESS) {
        int i;
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                RMH_PrintMsg("  Node %02u: %u\n", i, response.nodeValue[i]);
            }
        }
    }
    return ret;
}

static
RMH_Result RMH_Print_UINT32_NODEMESH(const RMH_Handle handle, RMH_Result (*api)(const RMH_Handle handle, RMH_NodeMesh_Uint32_t* response)) {
    RMH_NodeMesh_Uint32_t response;
    int i,j;

    RMH_Result ret = api(handle, &response);
    if (ret == RMH_SUCCESS) {
        char strBegin[256];
        char *strEnd=strBegin+sizeof(strBegin);
        char *str=strBegin;

        str+=snprintf(str, strEnd-str, "   ");
        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                str+=snprintf(str, strEnd-str, "  %02u ", i);
            }
        }
        RMH_PrintMsg("%s\n", strBegin);

        for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
            if (response.nodePresent[i]) {
                char *str=strBegin;
                RMH_NodeList_Uint32_t *nl = &response.nodeValue[i];
                str+=snprintf(str, strEnd-str, "%02u: ", i);
                for (j = 0; j < RMH_MAX_MOCA_NODES; j++) {
                    if (i == j) {
                        str+=snprintf(str, strEnd-str, " --  ");
                    } else if (response.nodePresent[j]) {
                        str+=snprintf(str, strEnd-str, "%04u ", nl->nodeValue[j]);
                    }
                }
                RMH_PrintMsg("%s\n", strBegin);
            }
        }
    }
    else {
        RMH_PrintMsg("%s\n", RMH_ResultToString(ret));
    }

    return ret;
}

#define PRINT_STATUS_MACRO(api, type, apiFunc, fmt, ...) { \
    type; \
    ret = apiFunc; \
    if (ret == RMH_SUCCESS) { RMH_PrintMsg("%-50s: " fmt "\n", #api, ##__VA_ARGS__); } \
    else                    { RMH_PrintMsg("%-50s: %s\n", #api,  RMH_ResultToString(ret)); } \
}
#define PRINT_STATUS_BOOL(api)              PRINT_STATUS_MACRO(api, bool response,                              api(handle, &response),                         "%s", response ? "TRUE" : "FALSE");
#define PRINT_STATUS_BOOL_RN(api, i)        PRINT_STATUS_MACRO(api, bool response,                              api(handle, i, &response),                      "%s", response ? "TRUE" : "FALSE");
#define PRINT_STATUS_UINT32(api)            PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, &response),                         "%u", response);
#define PRINT_STATUS_UINT32_RN(api, i)      PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, i, &response),                      "%u", response);
#define PRINT_STATUS_UINT32_HEX(api)        PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, &response),                         "0x%08x", response);
#define PRINT_STATUS_TABOO(api)             PRINT_STATUS_MACRO(api, uint32_t start;uint32_t mask,               api(handle, &start, &mask),                     "Start:%u Channel Mask:0x%08x", start, mask);
#define PRINT_STATUS_UINT32_HEX_RN(api, i)  PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, i, &response),                      "0x%08x", response);
#define PRINT_STATUS_UPTIME(api)            PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, &response),                         "%02uh:%02um:%02us", response/3600, (response%3600)/60, response%60);
#define PRINT_STATUS_STRING(api)            PRINT_STATUS_MACRO(api, char response[256],                         api(handle, response, sizeof(response)),        "%s", response);
#define PRINT_STATUS_STRING_RN(api, i)      PRINT_STATUS_MACRO(api, char response[256],                         api(handle, i, response, sizeof(response)),     "%s", response);
#define PRINT_STATUS_MAC(api)               PRINT_STATUS_MACRO(api, RMH_MacAddress_t response;char macStr[24],  api(handle, &response),                         "%s", RMH_MacToString(response, macStr, sizeof(macStr)));
#define PRINT_STATUS_MAC_RN(api, i)         PRINT_STATUS_MACRO(api, RMH_MacAddress_t response;char macStr[24],  api(handle, i, &response),                      "%s", RMH_MacToString(response, macStr, sizeof(macStr)));
#define PRINT_STATUS_MoCAVersion(api)       PRINT_STATUS_MACRO(api, RMH_MoCAVersion response,                   api(handle, &response),                         "%s", RMH_MoCAVersionToString(response));
#define PRINT_STATUS_MoCAVersion_RN(api, i) PRINT_STATUS_MACRO(api, RMH_MoCAVersion response,                   api(handle, i, &response),                      "%s", RMH_MoCAVersionToString(response));
#define PRINT_STATUS_PowerMode(api)         PRINT_STATUS_MACRO(api, RMH_PowerMode response;char outStr[128],    api(handle, &response),                         "%s", RMH_PowerModeToString(response, outStr, sizeof(outStr)));
#define PRINT_STATUS_PowerMode_RN(api, i)   PRINT_STATUS_MACRO(api, RMH_PowerMode response;char outStr[128],    api(handle, i, &response),                      "%s", RMH_PowerModeToString(response, outStr, sizeof(outStr)));
#define PRINT_STATUS_LinkStatus(api)        PRINT_STATUS_MACRO(api, RMH_LinkStatus response,                    api(handle, &response),                         "%s", RMH_LinkStatusToString(response));
#define PRINT_STATUS_FLOAT(api)             PRINT_STATUS_MACRO(api, float response,                             api(handle, &response),                         "%.02f", response);
#define PRINT_STATUS_FLOAT_RN(api, i)       PRINT_STATUS_MACRO(api, float response,                             api(handle,i, &response),                       "%.02f", response);
#define PRINT_STATUS_LOG_LEVEL(api)         PRINT_STATUS_MACRO(api, uint32_t response;char outStr[128],         api(handle, &response),                         "%s", RMH_LogLevelToString(response, outStr, sizeof(outStr)));
#define PRINT_STATUS_MAC_FLOW(api, mac)     PRINT_STATUS_MACRO(api, RMH_MacAddress_t response;char macStr[24],  api(handle, mac, &response),                    "%s", RMH_MacToString(response, macStr, sizeof(macStr)));
#define PRINT_STATUS_UINT32_FLOW(api, mac)  PRINT_STATUS_MACRO(api, uint32_t response,                          api(handle, mac, &response),                    "%u", response);

RMH_Result GENERIC_IMPL__RMH_Log_PrintStatus(const RMH_Handle handle, const char* filename) {
    RMH_Result ret;
    bool enabled;
    int i;

    if (filename) {
        handle->localLogToFile=fopen(filename, "a");
        if (handle->localLogToFile == NULL) {
            RMH_PrintErr("Failed to open '%s' for writing!\n", filename);
            return RMH_FAILURE;
        }
    }

    ret=RMH_Self_GetEnabled(handle, &enabled);
    if (ret != RMH_SUCCESS) {
        RMH_PrintErr("Failed calling RMH_Self_GetEnabled! Ensure the MoCA driver is properly loaded\n");
        return ret;
    }

    RMH_PrintMsg("= RMH Local Device Status ======\n");
    PRINT_STATUS_BOOL(RMH_Self_GetEnabled);
    PRINT_STATUS_STRING(RMH_Interface_GetName);
    PRINT_STATUS_MAC(RMH_Interface_GetMac);
    PRINT_STATUS_BOOL(RMH_Interface_GetEnabled);
    PRINT_STATUS_STRING(RMH_Self_GetSoftwareVersion);
    PRINT_STATUS_MoCAVersion(RMH_Self_GetHighestSupportedMoCAVersion);
    PRINT_STATUS_BOOL(RMH_Self_GetPreferredNCEnabled);
    PRINT_STATUS_UINT32(RMH_Self_GetLOF);
    PRINT_STATUS_BOOL(RMH_Power_GetTxPowerControlEnabled);
    PRINT_STATUS_BOOL(RMH_Power_GetTxBeaconPowerReductionEnabled);
    PRINT_STATUS_UINT32(RMH_Power_GetTxBeaconPowerReduction);
    PRINT_STATUS_TABOO(RMH_Self_GetTabooChannels);
    PRINT_STATUS_UINT32_HEX(RMH_Self_GetFrequencyMask);
    PRINT_STATUS_BOOL(RMH_Self_GetQAM256Enabled);
    PRINT_STATUS_BOOL(RMH_Self_GetTurboEnabled);
    PRINT_STATUS_BOOL(RMH_Self_GetBondingEnabled);
    PRINT_STATUS_BOOL(RMH_Self_GetPrivacyEnabled);
    PRINT_STATUS_STRING(RMH_Self_GetPrivacyMACManagementKey);
    PRINT_STATUS_LOG_LEVEL(RMH_Log_GetDriverLevel);
    PRINT_STATUS_STRING(RMH_Log_GetDriverFilename);
    PRINT_STATUS_PowerMode(RMH_Power_GetMode);

    if (enabled) {
        RMH_LinkStatus linkStatus;
        RMH_NodeList_Uint32_t nodes;

        ret=RMH_Self_GetLinkStatus(handle, &linkStatus);
        if (ret == RMH_SUCCESS && linkStatus == RMH_LINK_STATUS_UP) {
            RMH_PrintMsg("\n= Network Status ======\n");
            PRINT_STATUS_LinkStatus(RMH_Self_GetLinkStatus);
            PRINT_STATUS_UINT32(RMH_Network_GetNumNodes);
            PRINT_STATUS_UINT32(RMH_Network_GetNodeId);
            PRINT_STATUS_UINT32(RMH_Network_GetNCNodeId);
            PRINT_STATUS_MAC(RMH_Network_GetNCMac);
            PRINT_STATUS_UINT32(RMH_Network_GetBackupNCNodeId);
            PRINT_STATUS_UPTIME(RMH_Network_GetLinkUptime);
            PRINT_STATUS_MoCAVersion(RMH_Network_GetMoCAVersion);
            PRINT_STATUS_BOOL(RMH_Network_GetMixedMode);
            PRINT_STATUS_UINT32(RMH_Network_GetRFChannelFreq);
            PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalPackets);
            PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalPackets);
            PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalErrors);
            PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalErrors);
            PRINT_STATUS_UINT32(RMH_Stats_GetTxDroppedPackets);
            PRINT_STATUS_UINT32(RMH_Stats_GetRxDroppedPackets);
            ret=RMH_Network_GetRemoteNodeIds(handle, &nodes);
            if (ret == RMH_SUCCESS) {
                for (i = 0; i < RMH_MAX_MOCA_NODES; i++) {
                    if (nodes.nodePresent[i]) {
                        RMH_PrintMsg("\n= Remote Node ID %02u =======\n", i);
                        PRINT_STATUS_MAC_RN(RMH_RemoteNode_GetMac, i);
                        PRINT_STATUS_MoCAVersion_RN(RMH_RemoteNode_GetHighestSupportedMoCAVersion, i);
                        PRINT_STATUS_BOOL_RN(RMH_RemoteNode_GetPreferredNC, i);
                        PRINT_STATUS_UINT32_RN(RMH_RemoteNode_GetRxPackets, i);
                        PRINT_STATUS_UINT32_RN(RMH_RemoteNode_GetTxPackets, i);
                        PRINT_STATUS_UINT32_RN(RMH_RemoteNode_GetTxUnicastPhyRate, i);
                        PRINT_STATUS_UINT32_RN(RMH_RemoteNode_GetTxPowerReduction, i);
                        PRINT_STATUS_FLOAT_RN(RMH_RemoteNode_GetTxUnicastPower, i);
                        PRINT_STATUS_UINT32_RN(RMH_RemoteNode_GetRxTotalErrors, i);
                        PRINT_STATUS_FLOAT_RN(RMH_RemoteNode_GetRxUnicastPower, i);
                        PRINT_STATUS_FLOAT_RN(RMH_RemoteNode_GetRxSNR, i);
                    }
                }
            }

            RMH_PrintMsg("\n= PHY Rates =========\n");
            RMH_Print_UINT32_NODEMESH(handle, RMH_Network_GetTxUnicastPhyRate);
        }
        else {
            RMH_PrintMsg("*** MoCA link is down! ***\n");
        }
    }
    else {
        RMH_PrintMsg("*** MoCA not enabled! You may need to run 'rmh start' ***\n");
    }

    if (handle->localLogToFile) {
        fclose(handle->localLogToFile);
        handle->localLogToFile=NULL;
    }
    return RMH_SUCCESS;
}


RMH_Result GENERIC_IMPL__RMH_Log_PrintStats(const RMH_Handle handle, const char* filename) {
    RMH_Result ret;
    RMH_LinkStatus linkStatus;

    if (filename) {
        handle->localLogToFile=fopen(filename, "a");
        if (handle->localLogToFile == NULL) {
            RMH_PrintErr("Failed to open '%s' for writing!\n", filename);
            return RMH_FAILURE;
        }
    }

    ret=RMH_Self_GetLinkStatus(handle, &linkStatus);
    if (ret == RMH_SUCCESS && linkStatus == RMH_LINK_STATUS_UP) {
        RMH_PrintMsg("= Tx Stats ======\n");
        PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxUnicastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxBroadcastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxMulticastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxReservationRequestPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxMapPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxLinkControlPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxBeacons);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxDroppedPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalErrors);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalAggregatedPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetTxTotalBytes);

        RMH_PrintMsg("\n= Rx ======\n");
        PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalBytes);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxUnicastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxBroadcastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxMulticastPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxReservationRequestPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxMapPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxLinkControlPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxBeacons);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxUnknownProtocolPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxDroppedPackets);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalErrors);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxCRCErrors);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxTimeoutErrors);
        PRINT_STATUS_UINT32(RMH_Stats_GetRxTotalAggregatedPackets);
        RMH_PrintMsg("RMH_Stats_GetRxCorrectedErrors:\n");
        RMHApp__OUT_UINT32_NODELIST(handle, RMH_Stats_GetRxCorrectedErrors);
        RMH_PrintMsg("RMH_Stats_GetRxUncorrectedErrors:\n");
        RMHApp__OUT_UINT32_NODELIST(handle,RMH_Stats_GetRxUncorrectedErrors);
    }
    else {
            RMH_PrintMsg("*** Stats not available while MoCA link is down ***\n");
    }

/*RMH_Stats_GetRxPacketAggregation (const RMH_Handle handle, uint32_t* responseArray, const size_t responseArraySize, size_t* responseArrayUsed);
RMH_Stats_GetTxPacketAggregation (const RMH_Handle handle, uint32_t* responseArray, const size_t responseArraySize, size_t* responseArrayUsed);
RMH_Stats_GetRxCorrectedErrors (const RMH_Handle handle, RMH_NodeList_Uint32_t* response);
RMH_Stats_GetRxUncorrectedErrors (const RMH_Handle handle, RMH_NodeList_Uint32_t* response);*/

    if (handle->localLogToFile) {
        fclose(handle->localLogToFile);
        handle->localLogToFile=NULL;
    }
    return RMH_SUCCESS;
}


RMH_Result GENERIC_IMPL__RMH_Log_PrintFlows(const RMH_Handle handle, const char* filename) {
    RMH_Result ret;
    RMH_MacAddress_t flowIds[64];
    size_t numFlowIds;
    char macStr[24];
    uint32_t leaseTime;
    RMH_LinkStatus linkStatus;
    int i;

    if (filename) {
        handle->localLogToFile=fopen(filename, "a");
        if (handle->localLogToFile == NULL) {
            RMH_PrintErr("Failed to open '%s' for writing!\n", filename);
            return RMH_FAILURE;
        }
    }

    ret=RMH_Self_GetLinkStatus(handle, &linkStatus);
    if (ret == RMH_SUCCESS && linkStatus == RMH_LINK_STATUS_UP) {
        RMH_PrintMsg("= Local Flows ======\n");
        PRINT_STATUS_UINT32(RMH_PQoS_GetNumEgressFlows);
        PRINT_STATUS_UINT32(RMH_PQoS_GetNumIngressFlows);
        ret = RMH_PQoS_GetIngressFlowIds(handle, flowIds, sizeof(flowIds)/sizeof(flowIds[0]), &numFlowIds);
        if ((ret == RMH_SUCCESS) && (numFlowIds != 0)) {
            for (i=0; i < numFlowIds; i++) {
                RMH_PrintMsg("= Flow %u =======\n", i);
                RMH_PrintMsg("%-50s: %s\n", "Flow Id", RMH_MacToString(flowIds[i], macStr, sizeof(macStr)/sizeof(macStr[0])));
                PRINT_STATUS_MAC_FLOW(RMH_PQoSFlow_GetIngressMac, flowIds[i]);
                PRINT_STATUS_MAC_FLOW(RMH_PQoSFlow_GetEgressMac, flowIds[i]);
                PRINT_STATUS_MAC_FLOW(RMH_PQoSFlow_GetDestination, flowIds[i]);
                ret = RMH_PQoSFlow_GetLeaseTime(handle, flowIds[i], &leaseTime);
                if (ret != RMH_SUCCESS) {
                    RMH_PrintWrn("%-50s: %s\n", "RMH_PQoSFlow_GetLeaseTime", RMH_ResultToString(ret));
                }
                else {
                    if (leaseTime) {
                        RMH_PrintMsg("%-50s: %u\n", "RMH_PQoSFlow_GetLeaseTime", leaseTime);
                        PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetLeaseTimeRemaining, flowIds[i]);
                    }
                    else {
                        RMH_PrintMsg("%-50s: %s\n", "RMH_PQoSFlow_GetLeaseTime", "INFINITE");
                    }
                }

                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetPeakDataRate, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetBurstSize, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetFlowTag, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetPacketSize, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetMaxLatency, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetShortTermAvgRatio, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetMaxRetry, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetFlowPer, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetIngressClassificationRule, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetVLANTag, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetTotalTxPackets, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetDSCPMoCA, flowIds[i]);
                PRINT_STATUS_UINT32_FLOW(RMH_PQoSFlow_GetDFID, flowIds[i]);
            }
        }
    }
    else {
            RMH_PrintMsg("*** Flow information not available while MoCA link is down ***\n");
    }

    if (handle->localLogToFile) {
        fclose(handle->localLogToFile);
        handle->localLogToFile=NULL;
    }
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_Log_PrintModulation(const RMH_Handle handle, const char* filename) {
    uint32_t selfNodeId;
    uint32_t nodeId;
    RMH_MoCAVersion selfMoCAVersion;
    RMH_MoCAVersion remoteMoCAVersion;
    RMH_Result ret;
    RMH_NodeList_Uint32_t nodes;
    RMH_SubcarrierProfile p[4][512];
    uint32_t pUsed[4];
    RMH_LinkStatus linkStatus;

    ret=RMH_Self_GetLinkStatus(handle, &linkStatus);
    if (ret != RMH_SUCCESS || linkStatus != RMH_LINK_STATUS_UP) {
        RMH_PrintMsg("*** Modulation information not available while MoCA link is down ***\n");
        return ret;
    }

    ret = RMH_Network_GetNodeId(handle, &selfNodeId);
    if (ret != RMH_SUCCESS) {
        return ret;
    }

    ret = RMH_RemoteNode_GetActiveMoCAVersion(handle, selfNodeId, &selfMoCAVersion);
    if (ret != RMH_SUCCESS) {
        return ret;
    }

    ret = RMH_Network_GetRemoteNodeIds(handle, &nodes);
    if (ret != RMH_SUCCESS) {
        return ret;
    }

    RMH_PrintMsg("Subcarrier Modulation To/From The Self Node %d [MoCA %s]\n", selfNodeId, RMH_MoCAVersionToString(selfMoCAVersion));
    for (nodeId = 0; nodeId < RMH_MAX_MOCA_NODES; nodeId++) {
        if (nodes.nodePresent[nodeId]) {
            ret = RMH_RemoteNode_GetActiveMoCAVersion(handle, nodeId, &remoteMoCAVersion);
            if (ret != RMH_SUCCESS) {
                return ret;
            }

            RMH_PrintMsg("= Node ID %02u [MoCA %s] =======\n", nodeId, RMH_MoCAVersionToString(remoteMoCAVersion));
            if (selfMoCAVersion == RMH_MOCA_VERSION_20 && remoteMoCAVersion == RMH_MOCA_VERSION_20) {
                memset(pUsed, 0, sizeof(pUsed));
                ret = RMH_RemoteNode_GetRxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[0], sizeof(p[0])/sizeof(p[0][0]), &pUsed[0]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[1], sizeof(p[1])/sizeof(p[1][0]), &pUsed[1]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetRxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[2], sizeof(p[2])/sizeof(p[2][0]), &pUsed[2]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[3], sizeof(p[3])/sizeof(p[3][0]), &pUsed[3]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                RMH_PrintMsg("    Nominal Packet Error Rate [RMH_PER_MODE_NOMINAL]:\n");
                RMH_Print_MODULATION(handle, 256, 511,
                                                    "Primary Unicast Rx (NPER)", p[0], pUsed[0],
                                                    "Primary Unicast Tx (NPER)", p[1], pUsed[1],
                                                    "Broadcast Rx (NPER)", p[2], pUsed[2],
                                                    "Broadcast Tx (NPER)", p[3], pUsed[3]);

                RMH_Print_MODULATION(handle, 0, 255,
                                                    NULL, p[0], pUsed[0],
                                                    NULL, p[1], pUsed[1],
                                                    NULL, p[2], pUsed[2],
                                                    NULL, p[3], pUsed[3]);

                memset(pUsed, 0, sizeof(pUsed));
                ret = RMH_RemoteNode_GetRxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[0], sizeof(p[0])/sizeof(p[0][0]), &pUsed[0]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[1], sizeof(p[1])/sizeof(p[1][0]), &pUsed[1]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetRxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[2], sizeof(p[2])/sizeof(p[2][0]), &pUsed[2]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[3], sizeof(p[3])/sizeof(p[3][0]), &pUsed[3]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                RMH_PrintMsg("\n    Nominal Packet Error Rate [RMH_PER_MODE_VERY_LOW]:\n");
                RMH_Print_MODULATION(handle, 256, 511,
                                                    "Primary Unicast Rx (VLPER)", p[0], pUsed[0],
                                                    "Primary Unicast Tx (VLPER)", p[1], pUsed[1],
                                                    "Broadcast Rx (VLPER)", p[2], pUsed[2],
                                                    "Broadcast Tx (VLPER)", p[3], pUsed[3]);

                RMH_Print_MODULATION(handle, 0, 255,
                                                    NULL, p[0], pUsed[0],
                                                    NULL, p[1], pUsed[1],
                                                    NULL, p[2], pUsed[2],
                                                    NULL, p[3], pUsed[3]);



                memset(pUsed, 0, sizeof(pUsed));
                ret = RMH_RemoteNode_GetSecondaryRxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[0], sizeof(p[0])/sizeof(p[0][0]), &pUsed[0]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetSecondaryRxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetSecondaryRxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[1], sizeof(p[1])/sizeof(p[1][0]), &pUsed[1]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetSecondaryRxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetSecondaryTxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_NOMINAL, p[2], sizeof(p[2])/sizeof(p[2][0]), &pUsed[2]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetSecondaryTxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetSecondaryTxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_VERY_LOW, p[3], sizeof(p[3])/sizeof(p[3][0]), &pUsed[3]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetSecondaryTxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                RMH_PrintMsg("\n    Secondary Packet Error Rate:\n");
                RMH_Print_MODULATION(handle, 256, 511,
                                                    "Secondary Unicast Rx (NPER)", p[0], pUsed[0],
                                                    "Secondary Unicast Rx (VLPER)", p[1], pUsed[1],
                                                    "Secondary Unicast Tx (NPER)", p[2], pUsed[2],
                                                    "Secondary Unicast Tx (VLPER)", p[3], pUsed[3]);


                RMH_Print_MODULATION(handle, 0, 255,
                                                    NULL, p[0], pUsed[0],
                                                    NULL, p[1], pUsed[1],
                                                    NULL, p[2], pUsed[2],
                                                    NULL, p[3], pUsed[3]);

            }
            else {
                memset(pUsed, 0, sizeof(pUsed));
                ret = RMH_RemoteNode_GetRxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_LEGACY, p[0], sizeof(p[0])/sizeof(p[0][0]), &pUsed[0]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxUnicastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_LEGACY, p[1], sizeof(p[1])/sizeof(p[1][0]), &pUsed[1]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxUnicastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetRxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_LEGACY, p[2], sizeof(p[2])/sizeof(p[2][0]), &pUsed[2]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetRxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                ret = RMH_RemoteNode_GetTxBroadcastSubcarrierModulation(handle, nodeId, RMH_PER_MODE_LEGACY, p[3], sizeof(p[3])/sizeof(p[3][0]), &pUsed[3]);
                if (ret != RMH_SUCCESS && ret != RMH_UNIMPLEMENTED && ret != RMH_UNIMPLEMENTED) {
                    RMH_PrintWrn("RMH_RemoteNode_GetTxBroadcastSubcarrierModulation: %s\n", RMH_ResultToString(ret));
                }

                RMH_PrintMsg("    Packet Error Rate [RMH_PER_MODE_LEGACY]:\n");
                RMH_Print_MODULATION(handle, 127, 0,
                                                    "Unicast Rx", p[0], pUsed[0],
                                                    "Unicast Tx", p[1], pUsed[1],
                                                    "Broadcast Rx", p[2], pUsed[2],
                                                    "Broadcast Tx", p[3], pUsed[3]);

                RMH_Print_MODULATION(handle, 255, 128,
                                                    NULL, p[0], pUsed[0],
                                                    NULL, p[1], pUsed[1],
                                                    NULL, p[2], pUsed[2],
                                                    NULL, p[3], pUsed[3]);

            }
            RMH_PrintMsg("\n\n");
        }
    }
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_GetAllAPIs(const RMH_Handle handle, RMH_APIList** apiList) {
    *apiList=&hRMHGeneric_APIList;
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_GetUnimplementedAPIs(const RMH_Handle handle, RMH_APIList** apiList) {
    if (hRMHGeneric_SoCUnimplementedAPIList.apiListSize == 0) {
        int i;
        void* soclib = dlopen("librdkmocahalsoc.so.0", RTLD_LAZY);
        for(i=0; i != hRMHGeneric_APIList.apiListSize; i++) {
            RMH_API* api=hRMHGeneric_APIList.apiList[i];
            if (api->socApiExpected) {
                if (!soclib || !dlsym(soclib, api->socApiName)) {
                    hRMHGeneric_SoCUnimplementedAPIList.apiList[hRMHGeneric_SoCUnimplementedAPIList.apiListSize++]=api;
                }
            }
        }
        if (soclib) {
            dlclose(soclib);
        }
    }
    *apiList=&hRMHGeneric_SoCUnimplementedAPIList;
    return RMH_SUCCESS;
}

RMH_Result GENERIC_IMPL__RMH_GetAPITags(const RMH_Handle handle, RMH_APITagList** apiTagList) {
    /* If the tag list isn't initialized, do it now */
    if (hRMHGeneric_APITags.tagListSize == 0) {
        int i;
        RMH_APIList* tagList;
        char* token;

        for(i=0; i != hRMHGeneric_APIList.apiListSize; i++) {
            RMH_API* api=hRMHGeneric_APIList.apiList[i];
            char* tags=strdup(api->tags);
            if (!tags) {
                RMH_PrintWrn("Unable to copy tag string for '%s'. Skipping\n",  api->apiName);
                continue;
            }
            token = strtok(tags, ",");
            while(token) {
                if ((pRMH_FindTagList(handle, token, &tagList) == RMH_SUCCESS) && tagList) {
                    tagList->apiList[tagList->apiListSize++] = api;
                    RMH_PrintDbg("Added API '%s' to tag '%s'. Total APIs in this tags:%d\n", api->apiName, tagList->apiListName, tagList->apiListSize);
                }
                token = strtok(NULL, ",");
            }
            free(tags);
        }
        qsort(hRMHGeneric_APITags.tagList, hRMHGeneric_APITags.tagListSize, sizeof(RMH_APIList), pRMH_TagCompare);
    }
    *apiTagList=&hRMHGeneric_APITags;
    return RMH_SUCCESS;
}


RMH_Result GENERIC_IMPL__RMH_RemoteNode_GetNodeIdFromAssociatedId(RMH_Handle handle, const uint32_t associatedId, uint32_t* nodeId) {
    uint32_t associatedCounter=0;
    uint32_t nodeCounter;
    RMH_NodeList_Uint32_t remoteNodes;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(nodeId==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(associatedId>RMH_MAX_MOCA_NODES, RMH_INVALID_PARAM);
    BRMH_RETURN_IF_FAILED(RMH_Network_GetRemoteNodeIds(handle, &remoteNodes));

    for (nodeCounter = 0; nodeCounter < RMH_MAX_MOCA_NODES; nodeCounter++) {
        if (remoteNodes.nodePresent[nodeCounter]) {
            associatedCounter++;
            if (associatedCounter==associatedId) {
                *nodeId=nodeCounter;
                return RMH_SUCCESS;
            }
        }
    }
    return RMH_INVALID_ID;
}

RMH_Result GENERIC_IMPL__RMH_RemoteNode_GetAssociatedIdFromNodeId(RMH_Handle handle, const uint32_t nodeId, uint32_t* associatedId) {
    uint32_t associatedCounter=0;
    uint32_t nodeCounter;
    RMH_NodeList_Uint32_t remoteNodes;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(associatedId==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(nodeId>RMH_MAX_MOCA_NODES, RMH_INVALID_PARAM);
    BRMH_RETURN_IF_FAILED(RMH_Network_GetRemoteNodeIds(handle, &remoteNodes));

    for (nodeCounter = 0; nodeCounter < RMH_MAX_MOCA_NODES; nodeCounter++) {
        if (remoteNodes.nodePresent[nodeCounter]) {
            associatedCounter++;
            if (nodeCounter==nodeId) {
                *associatedId=associatedCounter;
                return RMH_SUCCESS;
            }
        }
    }
    return RMH_INVALID_ID;
}

RMH_Result GENERIC_IMPL__RMH_Network_GetAssociatedIds(RMH_Handle handle, RMH_NodeList_Uint32_t* response) {
    uint32_t associatedCounter=0;
    uint32_t nodeId;
    RMH_NodeList_Uint32_t remoteNodes;

    BRMH_RETURN_IF(handle==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF(response==NULL, RMH_INVALID_PARAM);
    BRMH_RETURN_IF_FAILED(RMH_Network_GetRemoteNodeIds(handle, &remoteNodes));

    memset(response, 0, sizeof(*response));
    for (nodeId = 0; nodeId < RMH_MAX_MOCA_NODES; nodeId++) {
        if (remoteNodes.nodePresent[nodeId]) {
            response->nodeValue[nodeId]=++associatedCounter;
            response->nodePresent[nodeId]=true;
        }
    }

    return RMH_SUCCESS;
}
