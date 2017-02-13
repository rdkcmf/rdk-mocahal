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
    AS(RMH_INVALID_ID, 4)\
    AS(RMH_TIMEOUT, 6)\
    AS(RMH_NOT_SUPPORTED, 7)\
    AS(RMH_UNIMPLEMENTED, 8)

#define ENUM_RMH_PowerMode \
    AS(RMH_POWER_MODE_0, 1u << 0)\
    AS(RMH_POWER_MODE_1, 1u << 1)\
    AS(RMH_POWER_MODE_2, 1u << 2)\
    AS(RMH_POWER_MODE_3, 1u << 3)

#define ENUM_RMH_LinkStatus \
    AS(RMH_NO_LINK, 0)\
    AS(RMH_INTERFACE_DOWN, 1)\
    AS(RMH_INTERFACE_UP, 2)

#define ENUM_RMH_MoCAVersion \
    AS(RMH_MOCA_VERSION_UNKNOWN, 0)\
    AS(RMH_MOCA_VERSION_10, 0x10)\
    AS(RMH_MOCA_VERSION_11, 0x11)\
    AS(RMH_MOCA_VERSION_20, 0x20)

#define ENUM_RMH_LogLevel \
    AS(RMH_LOG_LEVEL_ERROR, 0)\
    AS(RMH_LOG_LEVEL_MESSAGE, 1)\
    AS(RMH_LOG_LEVEL_DEBUG, 2)

/* No need to modify the enums here, they are populated from lists above */
#define AS(x,y) x=(y),
typedef enum { ENUM_RMH_Result } RMH_Result;
typedef enum { ENUM_RMH_LinkStatus } RMH_LinkStatus;
typedef enum { ENUM_RMH_PowerMode } RMH_PowerMode;
typedef enum { ENUM_RMH_MoCAVersion } RMH_MoCAVersion;
typedef enum { ENUM_RMH_LogLevel } RMH_LogLevel;
#undef AS

#define AS(x,y) #x,
const char * const RMH_ResultStr[] = { ENUM_RMH_Result };
static inline const char * const RMH_ResultToString(RMH_Result value) { return RMH_ResultStr[value]; }

const char * const RMH_LinkStatusStr[] = { ENUM_RMH_LinkStatus };
static inline const char * const RMH_LinkStatusToString(RMH_LinkStatus value) { return RMH_LinkStatusStr[value]; }

const char * const RMH_PowerModeStr[] = { ENUM_RMH_PowerMode };
static inline const char * const RMH_PowerModeToString(RMH_PowerMode value) { return RMH_PowerModeStr[value]; }

const char * const RMH_LogLevelStr[] = { ENUM_RMH_LogLevel };
static inline const char * const RMH_LogLevelToString(RMH_LogLevel value) { return RMH_LogLevelStr[value]; }

const char * const RMH_MoCAVersionStr[] = { ENUM_RMH_MoCAVersion };
static inline const char * const RMH_MoCAVersionToString(RMH_MoCAVersion value) {
    switch (value) {
        case RMH_MOCA_VERSION_UNKNOWN:
            return "Unknown";
            break;
        case RMH_MOCA_VERSION_10:
            return "1.0";
            break;
        case RMH_MOCA_VERSION_11:
            return "1.1";
            break;
        case RMH_MOCA_VERSION_20:
            return "2.0";
            break;
        default:
            return RMH_MoCAVersionStr[value];
            break;
    }
}

static inline const char * const
RMH_MacToString(const uint8_t mac[6], char* responseBuf, const size_t responseBufSize) {
    if (responseBuf) {
        int written=snprintf(responseBuf, responseBufSize, "%02X:%02X:%02X:%02X:%02X:%02X",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        if ((written > 0) && (written < responseBufSize))
            return responseBuf;
    }
    return NULL;
}
#undef AS

typedef struct RMH_NodeList_Uint32_t {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    uint32_t nodeValue[RMH_MAX_MOCA_NODES];
} RMH_NodeList_Uint32_t;

typedef struct RMH_NodeList_Mac {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    uint8_t nodeValue[RMH_MAX_MOCA_NODES][6];
} RMH_NodeList_Mac;

typedef struct RMH_NodeMesh_Uint32_t {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    RMH_NodeList_Uint32_t nodeValue[RMH_MAX_MOCA_NODES];
} RMH_NodeMesh_Uint32_t;


typedef enum RMH_Event {
    LINK_STATUS_CHANGED =       (1u << 0),
    MOCA_VERSION_CHANGED =      (1u << 1),
    RMH_LOG_PRINT =             (1u << 2)

} RMH_Event;

typedef struct RMH_EventData {
    union {
        struct {
            RMH_LinkStatus status;
        } LINK_STATUS_CHANGED;
        struct {
           RMH_MoCAVersion version;
        } MOCA_VERSION_CHANGED;
        struct {
            const char *logMsg;
        } RMH_LOG_PRINT;
    };
} RMH_EventData;


typedef void (*RMH_EventCallback)(enum RMH_Event event, struct RMH_EventData *eventData, void* userContext);

RMH_Handle RMH_Initialize();
void RMH_Destroy(RMH_Handle handle);

RMH_Result RMH_Callback_RegisterEvent(RMH_Handle handle, RMH_EventCallback eventCB, void* userContext, uint32_t eventNotifyBitMask);

RMH_Result RMH_Self_GetEnabled(RMH_Handle handle, bool *response);                                                  /* Check if MoCA software is active on the interface. Do not confuse with RMH_Interface_GetEnabled which checks if the low level interface is enabled */
RMH_Result RMH_Self_SetEnabled(RMH_Handle handle, const bool value);                                                /* Set if MoCA software is active on the interface. Do not confuse with RMH_Interface_SetEnabled which sets the low level interface enabled */
RMH_Result RMH_Self_GetNodeId(RMH_Handle handle, uint32_t* response);                                               /* Node ID of the local node.*/
RMH_Result RMH_Self_GetLOF(RMH_Handle handle, uint32_t *response);                                                  /* The Last Operating Frequency on which the Node formed the Network earlier */
RMH_Result RMH_Self_SetLOF(RMH_Handle handle, const uint32_t value);
RMH_Result RMH_Self_GetScanLOFOnly(RMH_Handle handle, bool *response);
RMH_Result RMH_Self_SetScanLOFOnly(RMH_Handle handle, const bool value);
RMH_Result RMH_Self_GetPreferredNCEnabled(RMH_Handle handle, bool* response);                                       /* Get the node preference to be Network Coordinator */
RMH_Result RMH_Self_SetPreferredNCEnabled(RMH_Handle handle, const bool value);                                     /* Set the node preference to be Network Coordinator */
RMH_Result RMH_Self_GetSoftwareVersion(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);         /* Firmware Version of the MoCA Firmware */
RMH_Result RMH_Self_GetHighestSupportedMoCAVersion(RMH_Handle handle, RMH_MoCAVersion* response);                   /* Highest Version of the MoCA Protocol that the Local Node Supports */
RMH_Result RMH_Self_GetFreqMask(RMH_Handle handle, uint32_t* response);                                             /* Get of Frequencies that should be used for forming network (bitmask) */
RMH_Result RMH_Self_SetFreqMask(RMH_Handle handle, const uint32_t value);                                           /* Set of Frequencies that should be used for forming network (bitmask) */
RMH_Result RMH_Self_GetPQOSEgressNumFlows(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_Self_GetMaxPacketAggr(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_Self_SetMaxPacketAggr(RMH_Handle handle, const uint32_t value);
RMH_Result RMH_Self_GetLowBandwidthLimit(RMH_Handle handle, uint32_t *response);
RMH_Result RMH_Self_SetLowBandwidthLimit(RMH_Handle handle, const uint32_t value);

RMH_Result RMH_Interface_GetEnabled(RMH_Handle handle, bool *response);                                             /* Check if the low level MoCA interface is enabled or disabled. Do not confuse with RMH_Self_GetEnabled which checks if the MoCA Software is active */
RMH_Result RMH_Interface_SetEnabled(RMH_Handle handle, const bool value);                                           /* Enable/Disable MoCA thelow level MoCA interface. Do not confuse with RMH_Interface_SetEnabled which sets the MoCA Software to be enabled. */
RMH_Result RMH_Interface_GetName(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);               /* Interface Name (for example: moca0) */
RMH_Result RMH_Interface_GetMac(RMH_Handle handle, uint8_t (*response)[6]);                                         /* MAC Address of the Local Node as hex bytes */

RMH_Result RMH_Network_GetLinkStatus(RMH_Handle handle, RMH_LinkStatus* status);                                    /* Current Status of the MoCA interface */
RMH_Result RMH_Network_GetNumNodes(RMH_Handle handle, uint32_t* response);                                          /* Number of Nodes on the MoCA Network - Cannot exceed the maximum supported by MoCA Protocol. */
RMH_Result RMH_Network_GetNodeId(RMH_Handle handle, RMH_NodeList_Uint32_t* response); /* Return the node ID for each node on the network. A fast way to get the node ids for all nodes INCLUDING the self node */
RMH_Result RMH_Network_GetAssociateId(RMH_Handle handle, RMH_NodeList_Uint32_t* response); /* Return the associate ID for each node on the network. A fast way to get the node ids for all nodes EXCLUDING the self node */
RMH_Result RMH_Network_GetNCNodeId(RMH_Handle handle, uint32_t* response);                                          /* Node ID of the Network Coordinator.*/
RMH_Result RMH_Network_GetBackupNCNodeId(RMH_Handle handle, uint32_t* response);                                    /* Node ID of the Backup Network Coordinator */
RMH_Result RMH_Network_GetNCMac(RMH_Handle handle, uint8_t (*response)[6]);                                         /* Network Coordinator MAC Address as hex bytes */
RMH_Result RMH_Network_GetLinkUptime(RMH_Handle handle, uint32_t* response);                                        /* Returns time the local node has been part of the network in seconds */
RMH_Result RMH_Network_GetMixedMode(RMH_Handle handle, bool *response);                                             /* Check if the MoCA interface is enabled or disabled */
RMH_Result RMH_Network_GetRFChannelFreq(RMH_Handle handle, uint32_t *response);                                     /* The Current Frequency on which the Node formed the Network */
RMH_Result RMH_Network_GetPrimaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetSecondaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetMoCAVersion(RMH_Handle handle, RMH_MoCAVersion* response);                                /* Current Operating MoCA Protocol Version */
RMH_Result RMH_Network_GetTxMapPhyRate(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Network_GetTxGcdPowerReduction(RMH_Handle handle, RMH_NodeList_Uint32_t* response);               /* Return the Gcd power reduction to all nodes in the network */

RMH_Result RMH_RemoteNode_GetNodeIdFromAssociateId(RMH_Handle handle, const uint8_t associatedId, uint32_t* response);
RMH_Result RMH_RemoteNode_GetAssociateIdFromNodeId(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNode_GetMac(RMH_Handle handle, const uint8_t nodeId, uint8_t (*response)[6]);                  /* MAC Address of a given nodeId as hex bytes */
RMH_Result RMH_RemoteNode_GetPreferredNC(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result RMH_RemoteNode_GetHighestSupportedMoCAVersion(RMH_Handle handle, const uint8_t nodeId, RMH_MoCAVersion* response);
RMH_Result RMH_RemoteNode_GetActiveMoCAVersion(RMH_Handle handle, const uint8_t nodeId, RMH_MoCAVersion* response);
RMH_Result RMH_RemoteNode_GetQAM256Capable(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result RMH_RemoteNode_GetMaxPacketAggr(RMH_Handle handle, const uint8_t nodeId, uint32_t *response);
RMH_Result RMH_RemoteNode_GetBondingEnabled(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result RMH_RemoteNode_GetConcat(RMH_Handle handle, const uint8_t nodeId, bool* response);

RMH_Result RMH_RemoteNodeRx_GetUnicastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeRx_GetBroadcastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeRx_GetPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result RMH_RemoteNodeRx_GetMapPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result RMH_RemoteNodeRx_GetPackets(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeRx_GetSNR(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result RMH_RemoteNodeRx_GetCorrectedErrors(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeRx_GetUnCorrectedErrors(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);

RMH_Result RMH_RemoteNodeTx_GetUnicastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeTx_GetUnicastPower(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeTx_GetBroadcastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result RMH_RemoteNodeTx_GetPower(RMH_Handle handle, const uint8_t nodeId, int32_t* response);
RMH_Result RMH_RemoteNodeTx_GetPowerReduction(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);

RMH_Result RMH_NetworkMesh_GetTxUnicastPhyRate(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response);                        /* Return the phy rates of all nodes in Mbps */
RMH_Result RMH_NetworkMesh_GetBitLoadingInfo(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response);

RMH_Result RMH_Power_GetMode(RMH_Handle handle, RMH_PowerMode* response);
RMH_Result RMH_Power_GetSupportedModes(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Power_GetTxPowerControlEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Power_SetTxPowerControlEnabled(RMH_Handle handle, const bool value);
RMH_Result RMH_Power_GetBeaconPowerReductionEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Power_SetBeaconPowerReductionEnabled(RMH_Handle handle, const bool value);
RMH_Result RMH_Power_GetBeaconPowerReduction(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Power_SetBeaconPowerReduction(RMH_Handle handle, const uint32_t value);
RMH_Result RMH_Power_GetTxGcdPowerReduction(RMH_Handle handle, uint32_t* response);                                  /* Return a generic Gcd power reduction value */

RMH_Result RMH_QAM256_GetCapable(RMH_Handle handle, bool* response);                                                /* Return if the local Node is QAM-256 Capable or Not */
RMH_Result RMH_QAM256_SetCapable(RMH_Handle handle, const bool value);
RMH_Result RMH_QAM256_GetTargetPhyRate(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_QAM256_SetTargetPhyRate(RMH_Handle handle, const uint32_t value);

RMH_Result RMH_Privacy_GetEnabled(RMH_Handle handle, bool* response);                                               /* Return if the link privacy is enabled. A Password is required when Privacy is enabled */
RMH_Result RMH_Privacy_SetEnabled(RMH_Handle handle, const bool value);                                             /* Enable/disable link privacy. A Password is required when Privacy is enabled */
RMH_Result RMH_Privacy_GetPassword(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);             /* Return the privacy password */
RMH_Result RMH_Privacy_SetPassword(RMH_Handle handle, const char* valueBuf);                                        /* Set the privacy password */

RMH_Result RMH_Turbo_GetEnabled(RMH_Handle handle, bool* response);
RMH_Result RMH_Turbo_SetEnabled(RMH_Handle handle, const bool value);

RMH_Result RMH_Stats_GetAdmissionAttempts(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionFailures(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionSucceeded(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetAdmissionsDeniedAsNC(RMH_Handle handle, uint32_t* response);

RMH_Result RMH_Stats_GetTxTotalBytes(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxTotalBytes(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxTotalPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxTotalPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxUnicastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxUnicastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxBroadcastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxBroadcastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxMulticastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxMulticastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxUnknownProtocolPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxDroppedPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxDroppedPackets(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxTotalErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxTotalErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxCRCErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxTimeoutErrors(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetRxMaxAggregation(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Stats_GetTxMaxAggregatedPackets(RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Stats_GetRxCorrectedErrors(RMH_Handle handle, RMH_NodeList_Uint32_t* response);
RMH_Result RMH_Stats_GetRxUncorrectedErrors(RMH_Handle handle, RMH_NodeList_Uint32_t* response);
RMH_Result RMH_Stats_Reset(RMH_Handle handle);

RMH_Result RMH_Taboo_GetStartChannel(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_Taboo_SetStartChannel(RMH_Handle handle, const uint32_t value);
RMH_Result RMH_Taboo_GetChannelMask(RMH_Handle handle, uint32_t* response);                                            /* Frequencies that the Local Node does not support (bitmask) */
RMH_Result RMH_Taboo_SetChannelMask(RMH_Handle handle, const uint32_t value);

RMH_Result RMH_Log_GetLevel(RMH_Handle handle, RMH_LogLevel* response);
RMH_Result RMH_Log_SetLevel(RMH_Handle handle, const RMH_LogLevel value);

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_H*/
