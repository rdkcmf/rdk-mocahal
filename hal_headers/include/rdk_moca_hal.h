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

#ifdef __cplusplus
extern "C" {
#endif

#include "rdk_moca_hal_types.h"
#include "rdk_moca_hal_soc.h"

/* API in this header are SoC generic. These functions will internally call other RMH APIs */

RMH_Result RMH_Start(RMH_Handle handle); /* Enable MoCA uisng Comcast specific configuration */
RMH_Result RMH_Stop(RMH_Handle handle);  /* Disable MoCA uisng Comcast specific configuration */
RMH_Result RMH_PrintStatus(RMH_Handle handle);/* Print a status summary of MoCA */
RMH_Result RMH_PrintFlows(RMH_Handle handle);
RMH_Result RMH_StatusWriteToFile(RMH_Handle handle, const char* filename);
RMH_Result RMH_Log_CreateNewLogFile(RMH_Handle handle, char* responseBuf, const size_t responseBufSize);
RMH_Result RMH_Interface_GetEnabled(RMH_Handle handle, bool *response);                                             /* Check if the low level MoCA interface is enabled or disabled. Do not confuse with RMH_Self_GetEnabled which checks if the MoCA Software is active */
RMH_Result RMH_Interface_SetEnabled(RMH_Handle handle, const bool value);                                           /* Enable/Disable MoCA thelow level MoCA interface. Do not confuse with RMH_Interface_SetEnabled which sets the MoCA Software to be enabled. */
RMH_Result RMH_Network_GetLinkStatus(RMH_Handle handle, RMH_LinkStatus* status);                                    /* Current Status of the MoCA interface */

RMH_Result RMH_PQoS_GetMaxEgressBandwidth(RMH_Handle handle, uint32_t* response);
RMH_Result RMH_PQoS_GetMinEgressBandwidth(RMH_Handle handle, uint32_t* response);

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_H*/
