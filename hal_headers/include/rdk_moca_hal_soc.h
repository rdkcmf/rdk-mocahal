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

#ifndef RDK_MOCA_HAL_SOC_H
#define RDK_MOCA_HAL_SOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rdk_moca_hal_types.h"

#ifndef __RMH_INIT_ATTRIB__
#define __RMH_INIT_ATTRIB__
#endif

#ifndef __RMH_DEST_ATTRIB__
#define __RMH_DEST_ATTRIB__
#endif

#ifndef __RMH_API_ATTRIB__
#define __RMH_API_ATTRIB__
#endif

RMH_Handle __RMH_INIT_ATTRIB__ RMH_Initialize(RMH_EventCallback eventCB, void* userContext);
RMH_Result __RMH_API_ATTRIB__  RMH_Destroy(RMH_Handle handle);

RMH_Result __RMH_API_ATTRIB__ RMH_Callback_RegisterEvents(RMH_Handle handle, uint32_t eventNotifyBitMask);
RMH_Result __RMH_API_ATTRIB__ RMH_Callback_UnregisterEvents(RMH_Handle handle, uint32_t eventNotifyBitMask);

/********************************************************
* Self APIs - Operate on the
********************************************************/
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetEnabled(RMH_Handle handle, bool *response);                                                  /* Check if MoCA software is active on the interface. Do not confuse with RMH_Interface_GetEnabled which checks if the low level interface is enabled */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetEnabled(RMH_Handle handle, const bool value);                                                /* Set if MoCA software is active on the interface. Do not confuse with RMH_Interface_SetEnabled which sets the low level interface enabled */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetMoCALinkUp(RMH_Handle handle, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetNodeId(RMH_Handle handle, uint32_t* response);                                               /* Node ID of the local node.*/
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetLOF(RMH_Handle handle, uint32_t *response);                                                  /* The Last Operating Frequency on which the Node formed the Network earlier */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetLOF(RMH_Handle handle, const uint32_t value);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetScanLOFOnly(RMH_Handle handle, bool *response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetScanLOFOnly(RMH_Handle handle, const bool value);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetPreferredNCEnabled(RMH_Handle handle, bool* response);                                       /* Get the node preference to be Network Coordinator */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetPreferredNCEnabled(RMH_Handle handle, const bool value);                                     /* Set the node preference to be Network Coordinator */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetSoftwareVersion(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);         /* Firmware Version of the MoCA Firmware */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetHighestSupportedMoCAVersion(RMH_Handle handle, RMH_MoCAVersion* response);                   /* Highest Version of the MoCA Protocol that the Local Node Supports */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetFreqMask(RMH_Handle handle, uint32_t* response);                                             /* Get of Frequencies that should be used for forming network (bitmask) */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetFreqMask(RMH_Handle handle, const uint32_t value);                                           /* Set of Frequencies that should be used for forming network (bitmask) */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetMaxPacketAggregation(RMH_Handle handle, uint32_t *response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetMaxPacketAggregation(RMH_Handle handle, const uint32_t value);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetLowBandwidthLimit(RMH_Handle handle, uint32_t *response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetLowBandwidthLimit(RMH_Handle handle, const uint32_t value);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetMaxBitrate(RMH_Handle handle, uint32_t *response);                                           /* The maximum PHY rate supported in non-turbo mode in Mbps */
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetTxBroadcastPhyRate(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_SetTxPowerLimit(RMH_Handle handle, const int32_t value);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetTxPowerLimit(RMH_Handle handle, int32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetSupportedFrequencies(RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize, uint32_t* responseBufUsed);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_RestoreDefaultSettings(RMH_Handle handle);

RMH_Result __RMH_API_ATTRIB__ RMH_Interface_GetName(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);               /* Interface Name (for example: moca0) */
RMH_Result __RMH_API_ATTRIB__ RMH_Interface_GetMac(RMH_Handle handle, RMH_MacAddress_t* response);                                     /* MAC Address of the Local Node as hex bytes */
RMH_Result __RMH_API_ATTRIB__ RMH_Interface_SetMac(RMH_Handle handle, const RMH_MacAddress_t value);

RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetNumNodes(RMH_Handle handle, uint32_t* response);                                          /* Number of Nodes on the MoCA Network - Cannot exceed the maximum supported by MoCA Protocol. */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetNodeId(RMH_Handle handle, RMH_NodeList_Uint32_t* response); /* Return the node ID for each node on the network. A fast way to get the node ids for all nodes INCLUDING the self node */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetRemoteNodeId(RMH_Handle handle, RMH_NodeList_Uint32_t* response);                         /* Return the node ID for each remote node on the network. A fast way to get the node ids for all nodes EXCLUDING the self node */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetAssociatedId(RMH_Handle handle, RMH_NodeList_Uint32_t* response); /* Return the Associated ID for each node on the network. A fast way to get the node ids for all nodes EXCLUDING the self node */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetNCNodeId(RMH_Handle handle, uint32_t* response);                                          /* Node ID of the Network Coordinator.*/
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetBackupNCNodeId(RMH_Handle handle, uint32_t* response);                                    /* Node ID of the Backup Network Coordinator */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetNCMac(RMH_Handle handle, RMH_MacAddress_t* response);                                     /* Network Coordinator MAC Address as hex bytes */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetLinkUptime(RMH_Handle handle, uint32_t* response);                                        /* Returns time the local node has been part of the network in seconds */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetMixedMode(RMH_Handle handle, bool *response);                                             /* Check if the MoCA interface is enabled or disabled */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetRFChannelFreq(RMH_Handle handle, uint32_t *response);                                     /* The Current Frequency on which the Node formed the Network */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetPrimaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetSecondaryChannelFreq(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetMoCAVersion(RMH_Handle handle, RMH_MoCAVersion* response);                                /* Current Operating MoCA Protocol Version */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetTxMapPhyRate(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetTxGcdPowerReduction(RMH_Handle handle, uint32_t* response);                               /* Return the Gcd power reduction of the network */
RMH_Result __RMH_API_ATTRIB__ RMH_Network_GetTabooMask(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_NetworkMesh_GetTxUnicastPhyRate(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response);                        /* Return the phy rates of all nodes in Mbps */
RMH_Result __RMH_API_ATTRIB__ RMH_NetworkMesh_GetBitLoadingInfo(RMH_Handle handle, RMH_NodeMesh_Uint32_t* response);

RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetNodeIdFromAssociatedId(RMH_Handle handle, const uint8_t associatedId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetAssociatedIdFromNodeId(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetMac(RMH_Handle handle, const uint8_t nodeId, RMH_MacAddress_t* response);              /* MAC Address of a given nodeId as hex bytes */
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetPreferredNC(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetHighestSupportedMoCAVersion(RMH_Handle handle, const uint8_t nodeId, RMH_MoCAVersion* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetActiveMoCAVersion(RMH_Handle handle, const uint8_t nodeId, RMH_MoCAVersion* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetQAM256Capable(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetMaxPacketAggregation(RMH_Handle handle, const uint8_t nodeId, uint32_t *response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetBondingCapable(RMH_Handle handle, const uint8_t nodeId, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNode_GetConcat(RMH_Handle handle, const uint8_t nodeId, bool* response);

RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetUnicastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetBroadcastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetBroadcastPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetUnicastPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetMapPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetPackets(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetSNR(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetCorrectedErrors(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetUnCorrectedErrors(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeRx_GetTotalErrors(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);

RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeTx_GetUnicastPhyRate(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeTx_GetUnicastPower(RMH_Handle handle, const uint8_t nodeId, float* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeTx_GetPowerReduction(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_RemoteNodeTx_GetPackets(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);

RMH_Result __RMH_API_ATTRIB__ RMH_Power_GetMode(RMH_Handle handle, RMH_PowerMode* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_GetSupportedModes(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_GetTxPowerControlEnabled(RMH_Handle handle, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_SetTxPowerControlEnabled(RMH_Handle handle, const bool value);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_GetTxBeaconPowerReductionEnabled(RMH_Handle handle, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_SetTxBeaconPowerReductionEnabled(RMH_Handle handle, const bool value);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_GetTxBeaconPowerReduction(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Power_SetTxBeaconPowerReduction(RMH_Handle handle, const uint32_t value);

RMH_Result __RMH_API_ATTRIB__ RMH_QAM256_GetEnabled(RMH_Handle handle, bool* response);                                                /* Return if the self node is QAM-256 Capable or not. May return RMH_NOT_SUPPORTED */
RMH_Result __RMH_API_ATTRIB__ RMH_QAM256_SetEnabled(RMH_Handle handle, const bool value);                                              /* Set if the self node should enable QAM-256. May return RMH_NOT_SUPPORTED */
RMH_Result __RMH_API_ATTRIB__ RMH_QAM256_GetTargetPhyRate(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_QAM256_SetTargetPhyRate(RMH_Handle handle, const uint32_t value);

RMH_Result __RMH_API_ATTRIB__ RMH_Privacy_GetEnabled(RMH_Handle handle, bool* response);                                               /* Return if the link privacy is enabled. A Password is required when Privacy is enabled */
RMH_Result __RMH_API_ATTRIB__ RMH_Privacy_SetEnabled(RMH_Handle handle, const bool value);                                             /* Enable/disable link privacy. A Password is required when Privacy is enabled */
RMH_Result __RMH_API_ATTRIB__ RMH_Privacy_GetPassword(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);             /* Return the privacy password */
RMH_Result __RMH_API_ATTRIB__ RMH_Privacy_SetPassword(RMH_Handle handle, const char* value);                                           /* Set the privacy password */

RMH_Result __RMH_API_ATTRIB__ RMH_Turbo_GetEnabled(RMH_Handle handle, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Turbo_SetEnabled(RMH_Handle handle, const bool value);

RMH_Result __RMH_API_ATTRIB__ RMH_Bonding_GetEnabled(RMH_Handle handle, bool* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Bonding_SetEnabled(RMH_Handle handle, const bool value);

RMH_Result __RMH_API_ATTRIB__ RMH_Taboo_GetStartChannel(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Taboo_SetStartChannel(RMH_Handle handle, const uint32_t value);
RMH_Result __RMH_API_ATTRIB__ RMH_Taboo_GetChannelMask(RMH_Handle handle, uint32_t* response);                                            /* Frequencies that the Local Node does not support (bitmask) */
RMH_Result __RMH_API_ATTRIB__ RMH_Taboo_SetChannelMask(RMH_Handle handle, const uint32_t value);

RMH_Result __RMH_API_ATTRIB__ RMH_PQoS_GetNumIngressFlows(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Self_GetPQOSEgressNumFlows(RMH_Handle handle, uint32_t* response); /* To be renamed RMH_PQoS_GetNumEgressFlows */
RMH_Result __RMH_API_ATTRIB__ RMH_PQoS_GetEgressBandwidth(RMH_Handle handle, const uint8_t nodeId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoS_GetIngressFlowIds(RMH_Handle handle, RMH_MacAddress_t* responseBuf, const size_t responseBufSize, size_t* responseBufUsed);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetPeakDataRate(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetBurstSize(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetLeaseTime(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetLeaseTimeRemaining(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetFlowTag(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetMaxLatency(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetShortTermAvgRatio(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetMaxRetry(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetVLANTag(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetFlowPer(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetIngressClassificationRule(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetPacketSize(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetTotalTxPackets(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetDSCPMoCA(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetDFID(RMH_Handle handle, const RMH_MacAddress_t flowId, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetDestination(RMH_Handle handle, const RMH_MacAddress_t flowId, RMH_MacAddress_t *response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetIngressMac(RMH_Handle handle, const RMH_MacAddress_t flowId, RMH_MacAddress_t *response);
RMH_Result __RMH_API_ATTRIB__ RMH_PQoSFlow_GetEgressMac(RMH_Handle handle, const RMH_MacAddress_t flowId, RMH_MacAddress_t *response);

RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetAdmissionAttempts(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetAdmissionFailures(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetAdmissionSucceeded(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetAdmissionsDeniedAsNC(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxTotalBytes(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxTotalBytes(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxTotalPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxTotalPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxUnicastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxUnicastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxBroadcastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxBroadcastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxMulticastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxMulticastPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxReservationRequestPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxReservationRequestPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxMapPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxMapPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxLinkControlPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxLinkControlPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxBeacons(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxBeacons(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxUnknownProtocolPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxDroppedPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxDroppedPackets(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxTotalErrors(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxTotalErrors(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxCRCErrors(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxTimeoutErrors(RMH_Handle handle, uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxTotalAggregatedPackets(RMH_Handle handle, uint32_t* response); /* Maximum total number of aggregated packets */
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxTotalAggregatedPackets(RMH_Handle handle, uint32_t* response); /* Maximum total number of aggregated packets */
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxPacketAggregation(RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize, uint32_t* responseBufUsed);  /* Return Rx packet aggregation. Index 0 is packets without aggregation. Index 1 is 2 pkt aggregation.  Index 2 is 3 pkt aggregation. And so on */
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetTxPacketAggregation(RMH_Handle handle, uint32_t* responseBuf, const size_t responseBufSize, uint32_t* responseBufUsed);  /* Return Rx packet aggregation. Index 0 is packets without aggregation. Index 1 is 2 pkt aggregation.  Index 2 is 3 pkt aggregation. And so on */
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxCorrectedErrors(RMH_Handle handle, RMH_NodeList_Uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_GetRxUncorrectedErrors(RMH_Handle handle, RMH_NodeList_Uint32_t* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Stats_Reset(RMH_Handle handle);

RMH_Result __RMH_API_ATTRIB__ RMH_Log_GetLevel(RMH_Handle handle, RMH_LogLevel* response);
RMH_Result __RMH_API_ATTRIB__ RMH_Log_SetLevel(RMH_Handle handle, const RMH_LogLevel value);
RMH_Result __RMH_API_ATTRIB__ RMH_Log_GetFilename(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result __RMH_API_ATTRIB__ RMH_Log_SetFilename(RMH_Handle handle, const char* value);

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_SOC_H*/
