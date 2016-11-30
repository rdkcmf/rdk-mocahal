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
#include "moca_hal.h"

#define MOCA_API_SUCCESS 1
#define MOCA_API_ERROR 0
int moca_checkInit()
{
    int retVal=0;
    if(!mocaContext)
    {
        retVal=moca_init();
        if(retVal == -1)
        {
            printf("Issue in initializing MoCA");
            return retVal;
        }
    }
    
}

int moca_init(){
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    int retVal=0;
    return retVal
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
}

int moca_getStatus(MoCAOperStatus *status)
{
    int retVal=0;
    *status=DOWN;
    return retVal; 
}

int moca_getNetworkControllerNodeId(unsigned int *nodeID)
{
    int retVal=0;
    *nodeID = 1;
    return retVal;
}       

int moca_getNetworkControllerMac(unsigned char * mac)
{
    unsigned int i;
    int retVal=MOCA_API_SUCCESS;
    char macaddress[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    for (i = 0; i < 6; i++)
            mac[i] = macaddress[i];
    return retVal;
}
int moca_checkInit()
{
    int retVal=0;
    if(!mocaContext)
    {
        retVal=moca_init();
        if(retVal == -1)
        {
            printf("Issue in initializing MoCA");
            return retVal;
        }
    }
}
int moca_getNumberOfNodesConnected(unsigned int *totalMoCANode)
{
    int retVal=MOCA_API_ERROR;
    MoCAOperStatus *status;
    if((MOCA_API_SUCCESS == moca_getStatus(status)) && (status == UP))
    {
        *totalMoCANode=2;
        retVal = MOCA_API_SUCCESS;
    }
    return retVal;
}


int moca_getPhyRxRate(unsigned int * phyRxRate)
{
    	int retVal=MOCA_API_ERROR;
	*phyRxRate=100;
	retVal = MOCA_API_SUCCESS;
}
int moca_getPhyTxRate(unsigned int * phyTxRate)
{
    	int retVal=MOCA_API_ERROR;
	*phyTxRate=100;
	retVal = MOCA_API_SUCCESS;
}
