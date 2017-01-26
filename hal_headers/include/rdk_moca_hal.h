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

#ifndef RDK_MOCA_HAL_H
#define RDK_MOCA_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RMH_MAX_MOCA_NODES 16

typedef struct RMH* RMH_Handle;


#define ENUM_RMH_Result \
    AS(RMH_SUCCESS, 0)\
    AS(RMH_FAILURE, 1)\
    AS(RMH_INVALID_PARAM, 2)\
    AS(RMH_INVALID_NETWORK_STATE, 3)\
    AS(RMH_INVALID_INTERNAL_STATE, 4)\
    AS(RMH_TIMEOUT, 5)\
    AS(RMH_NOT_SUPPORTED, 6)\
    AS(RMH_UNIMPLEMENTED, 7)

#define ENUM_RMH_PowerMode \
    AS(RMH_POWER_MODE_0, 1u << 0)\
    AS(RMH_POWER_MODE_1, 1u << 1)\
    AS(RMH_POWER_MODE_2, 1u << 2)\
    AS(RMH_POWER_MODE_3, 1u << 3)

#define ENUM_RMH_LinkStatus \
    AS(RMH_NO_LINK, 0)\
    AS(RMH_INTERFACE_DOWN, 1)\
    AS(RMH_INTERFACE_UP, 2)


/* No need to modify the enums here, they are populated from lists above */
#define AS(x,y) x=(y),
typedef enum { ENUM_RMH_Result } RMH_Result;
typedef enum { ENUM_RMH_LinkStatus } RMH_LinkStatus;
typedef enum { ENUM_RMH_PowerMode } RMH_PowerMode;
#undef AS

#define AS(x,y) #x,
const char * const RMH_ResultStr[] = { ENUM_RMH_Result };
const char * const RMH_LinkStatusStr[] = { ENUM_RMH_LinkStatus };
const char * const RMH_PowerModeStr[] = { ENUM_RMH_PowerMode };
#undef AS

typedef enum RMH_Event {
    LINK_STATUS_CHANGED =       (1u << 0),
    MOCA_VERSION_CHANGED =      (1u << 1)
} RMH_Event;

typedef struct RMH_EventData {
    union {
        struct {
            RMH_LinkStatus status;
        } LINK_STATUS_CHANGED;
        struct {
           char version[32];
           uint32_t versionId;
        } MOCA_VERSION_CHANGED;
    };
} RMH_EventData;

typedef struct RMH_RemoteNodeStatus{
    uint32_t nodeId;
    uint32_t neighNodeId;
    uint32_t phyRate;
    int32_t power;
    uint32_t backoff;
} RMH_RemoteNodeStatus;

typedef struct RMH_NetworkStatus {
    uint32_t localNodeId;
    uint32_t localNeighNodeId;
    uint32_t ncNodeId;
    uint32_t ncNeighNodeId;
    uint32_t remoteNodesPresent;
    RMH_RemoteNodeStatus remoteNodes[RMH_MAX_MOCA_NODES];
} RMH_NetworkStatus;


typedef struct RMH_NodeBitLoadingInfo {
    uint32_t nodeId;
    struct {
        uint32_t nodeId;
        uint16_t bitloading;
    } connectedNodes[RMH_MAX_MOCA_NODES];
} RMH_NodeBitLoadingInfo;

typedef struct RMH_NetworkBitLoadingInfo {
    uint32_t remoteNodesPresent;
    RMH_NodeBitLoadingInfo remoteNodes[RMH_MAX_MOCA_NODES];
} RMH_NetworkBitLoadingInfo;

typedef void (*RMH_EventCallback)(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext);

RMH_Handle RMH_Initialize();
void RMH_Destroy(RMH_Handle handle);

RMH_Result RMH_Callback_RegisterEvent(RMH_Handle handle, RMH_EventCallback eventCB, void* userContext, uint32_t eventNotifyBitMask);

RMH_Result RMH_Self_GetMac(RMH_Handle handle, uint8_t (*response)[6]);
RMH_Result RMH_Self_GetMacString(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Self_GetLOF(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_Self_GetNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Self_GetPreferredNCEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Self_SetPreferredNCEnabled(RMH_Handle handle, const bool value);
RMH_Result RMH_Self_GetSoftwareVersion(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Self_GetHighestSupportedMoCAVersion(RMH_Handle handle, char* responseBuf, const size_t verBufSize);

RMH_Result RMH_Network_GetLinkStatus(RMH_Handle handle, RMH_LinkStatus* status);
RMH_Result RMH_Network_GetRFChannelFreq(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_Network_GetPrimaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetSecondaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetNCNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetBackupNCNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetNumNodes(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetLinkUptime(RMH_Handle handle, uint32_t* response); /* Returns up time in seconds */
RMH_Result RMH_Network_GetNCMac(RMH_Handle handle, uint8_t (*response)[6]);
RMH_Result RMH_Network_GetNodeMac(RMH_Handle handle, const uint8_t nodeId, uint8_t (*response)[6]);
RMH_Result RMH_Network_GetNCMacString(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Network_GetNodeMacString(RMH_Handle handle, const uint8_t nodeId, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Network_GetMoCAVersion(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Network_GetStatusTx(RMH_Handle handle, RMH_NetworkStatus* response);
RMH_Result RMH_Network_GetStatusRx(RMH_Handle handle, RMH_NetworkStatus* response);
RMH_Result RMH_Network_GetBitLoadingInfo(RMH_Handle handle, RMH_NetworkBitLoadingInfo* response);

RMH_Result RMH_Power_GetMode(RMH_Handle handle, RMH_PowerMode* response);
RMH_Result RMH_Power_GetSupportedModes(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Power_GetBeaconPowerReductionEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Power_SetBeaconPowerReductionEnabled(RMH_Handle handle, const bool value);
RMH_Result RMH_Power_GetBeaconPowerReduction(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Power_SetBeaconPowerReduction(RMH_Handle handle, const uint32_t value);

RMH_Result RMH_QAM256_GetCapable(RMH_Handle handle, bool* response);
RMH_Result RMH_QAM256_SetCapable(RMH_Handle handle, const bool value);

RMH_Result RMH_Privacy_GetPassword(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Privacy_SetPassword(RMH_Handle handle, const char* valueBuf);
RMH_Result RMH_Privacy_GetEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Privacy_SetEnabled(RMH_Handle handle, const bool value);

RMH_Result RMH_Turbo_GetEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Turbo_SetEnabled(RMH_Handle handle, const bool value);

RMH_Result RMH_Stats_GetFrameCRCErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetFrameTimeoutErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionAttempts(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionFailures(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionSucceeded(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionsDeniedAsNC(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_Reset(RMH_Handle handle);


static inline const char * const RMH_ResultToString(RMH_Result value) { return RMH_ResultStr[value]; }
static inline const char * const RMH_PowerModeToString(RMH_PowerMode value) { return RMH_PowerModeStr[value]; }

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_H*/
