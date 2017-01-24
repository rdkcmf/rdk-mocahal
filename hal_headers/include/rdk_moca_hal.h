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

#define RMH_RESULT_LIST \
    AS(RMH_SUCCESS)\
    AS(RMH_FAILURE)\
    AS(RMH_INVALID_PARAM)\
    AS(RMH_INVALID_NETWORK_STATE)\
    AS(RMH_INVALID_INTERNAL_STATE)\
    AS(RMH_TIMEOUT)\
    AS(RMH_NOT_SUPPORTED)\
    AS(RMH_UNIMPLEMENTED)


/* No need to modify the enums here, they are populated from lists above */
#define AS(x) x,
typedef enum RMH_Result { RMH_RESULT_LIST } RMH_Result;
#undef AS

#define AS(x) #x,
const char * const RMH_ResultStr[] = { RMH_RESULT_LIST };
#undef AS

typedef enum RMH_LinkStatus {
    RMH_NO_LINK = 0,
    RMH_INTERFACE_DOWN,
    RMH_INTERFACE_UP,
} RMH_LinkStatus;

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

typedef void (*RMH_EventCallback)(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext);

RMH_Handle RMH_Initialize();
void RMH_Destroy(RMH_Handle handle);


RMH_Result RMH_RegisterEventCallback(RMH_Handle handle, RMH_EventCallback eventCB, void* userContext, uint32_t eventNotifyBitMask);

/* Read Only APIs */
RMH_Result RMH_GetLinkStatus(RMH_Handle handle, RMH_LinkStatus* status);

RMH_Result RMH_GetNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_GetNetworkControllerNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_GetBackupNetworkControllerNodeId(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_GetNumNodesInNetwork(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_GetRFChannel(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_GetLOF(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_GetLinkUptime(RMH_Handle handle, uint32_t* response); /* Returns up time in seconds */

RMH_Result RMH_GetQAM256Capable(RMH_Handle handle, bool* response);

RMH_Result RMH_GetMac(RMH_Handle handle, uint8_t (*response)[6]);
RMH_Result RMH_GetNetworkControllerMac(RMH_Handle handle, uint8_t (*response)[6]);

RMH_Result RMH_GetSoftwareVersion(RMH_Handle handle, uint8_t* responseBuf, const size_t responseBufSize);
RMH_Result RMH_GetMacString(RMH_Handle handle, uint8_t* responseBuf, const size_t responseBufSize);
RMH_Result RMH_GetNetworkControllerMacString(RMH_Handle handle, uint8_t* responseBuf, const size_t responseBufSize);
RMH_Result RMH_GetActiveMoCAVersion(RMH_Handle handle, uint8_t* responseBuf, const size_t responseBufSize);
RMH_Result RMH_GetHigestMoCAVersion(RMH_Handle handle, uint8_t* responseBuf, const size_t verBufSize);

RMH_Result RMH_GetNetworkStatusTx(RMH_Handle handle, RMH_NetworkStatus* response);
RMH_Result RMH_GetNetworkStatusRx(RMH_Handle handle, RMH_NetworkStatus* response);


/* Read/Write APIs */
RMH_Result RMH_GetPassword(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_SetPassword(RMH_Handle handle, const char* valueBuf);

RMH_Result RMH_GetPreferredNC(RMH_Handle handle, bool* response);
RMH_Result RMH_SetPreferredNC(RMH_Handle handle, const bool value);

RMH_Result RMH_GetPrivacyEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_SetPrivacyEnabled(RMH_Handle handle, const bool value);

RMH_Result RMH_GetBeaconPowerReductionEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_SetBeaconPowerReductionEnabled(RMH_Handle handle, const bool value);

RMH_Result RMH_GetBeaconPowerReduction(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_SetBeaconPowerReduction(RMH_Handle handle, const uint32_t value);

static inline const char * const RMH_ResultToString(RMH_Result result) { return RMH_ResultStr[result]; }

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_H*/
