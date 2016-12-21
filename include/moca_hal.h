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

typedef enum _eMoCALinkStatus {
    DOWN,
    UP
} eMoCALinkStatus;

typedef struct _mocaPhyRateData
{
    unsigned int neighNodeId[15];
    unsigned int phyRate[15];
    unsigned int totalNodesPresent;
    unsigned int deviceNodeId;
} mocaPhyRateData;

typedef struct _mocaPowerLevelData
{
    unsigned int neighNodeId[15];
    int mocaPower[15];
    unsigned int totalNodesPresent;
    unsigned int deviceNodeId;
} mocaPowerLevelData;

int moca_init();
int moca_UnInit();
int moca_getStatus(eMoCALinkStatus *status);
int moca_getNetworkControllerNodeId(unsigned int *nodeID);
int moca_getNetworkControllerMac(unsigned char * mac);
int moca_checkInit();
int moca_getNumberOfNodesConnected(unsigned int *totalMoCANode);
int moca_getPhyTxRate(mocaPhyRateData * mocaPhyRate);
int moca_getPhyRxRate(mocaPhyRateData * mocaPhyRate);
int moca_getTxPowerReduction(mocaPowerLevelData * mocaPowerLevel);
int moca_getRxPower(mocaPowerLevelData * mocaPowerLevel);
int get_phy_rate(void * ctx, unsigned int node, unsigned int profile, unsigned int *phy_rate);
int get_power_level(void * ctx, unsigned int node, unsigned int profile, int *power_level,int isRx);
