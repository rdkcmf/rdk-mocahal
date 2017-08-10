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

#ifndef RDK_MOCA_HAL_TYPES_H
#define RDK_MOCA_HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RMH_MAX_NUM_APIS 1024
#define RMH_MAX_NUM_TAGS 32
#define RMH_MAX_MOCA_NODES 16
typedef struct RMH* RMH_Handle;
typedef uint8_t RMH_MacAddress_t[6];

#define AS(x,y) x=(y),
#define ENUM_RMH_Result \
    AS(RMH_SUCCESS,                         0) \
    AS(RMH_FAILURE,                         1) \
    AS(RMH_INVALID_PARAM,                   2) \
    AS(RMH_INVALID_NETWORK_STATE,           3) \
    AS(RMH_INVALID_INTERNAL_STATE,          4) \
    AS(RMH_INVALID_ID,                      5) \
    AS(RMH_TIMEOUT,                         6) \
    AS(RMH_INSUFFICIENT_SPACE,              7) \
    AS(RMH_NOT_SUPPORTED,                   8) \
    AS(RMH_UNIMPLEMENTED,                   9)
typedef enum RMH_Result { ENUM_RMH_Result } RMH_Result;

#define ENUM_RMH_PowerMode \
    AS(RMH_POWER_MODE_0,                    1u << 0) \
    AS(RMH_POWER_MODE_1,                    1u << 1) \
    AS(RMH_POWER_MODE_2,                    1u << 2) \
    AS(RMH_POWER_MODE_3,                    1u << 3)
typedef enum RMH_PowerMode { ENUM_RMH_PowerMode } RMH_PowerMode;

#define ENUM_RMH_LinkStatus \
    AS(RMH_LINK_STATUS_DISABLED,        0) \
    AS(RMH_LINK_STATUS_NO_LINK,         1) \
    AS(RMH_LINK_STATUS_INTERFACE_DOWN,  2) \
    AS(RMH_LINK_STATUS_UP,              3)
typedef enum RMH_LinkStatus { ENUM_RMH_LinkStatus } RMH_LinkStatus;

#define ENUM_RMH_AdmissionStatus \
    AS(RMH_ADMISSION_STATUS_UNKNOWN,            0) \
    AS(RMH_ADMISSION_STATUS_STARTED,            1) \
    AS(RMH_ADMISSION_STATUS_SUCCEEDED,          2) \
    AS(RMH_ADMISSION_STATUS_FAILED,             3) \
    AS(RMH_ADMISSION_STATUS_CHANNEL_UNUSABLE,   4) \
    AS(RMH_ADMISSION_STATUS_TIMEOUT,            5)
typedef enum RMH_AdmissionStatus { ENUM_RMH_AdmissionStatus } RMH_AdmissionStatus;

#define ENUM_RMH_MoCAVersion \
    AS(RMH_MOCA_VERSION_UNKNOWN,            0) \
    AS(RMH_MOCA_VERSION_10,                 0x10) \
    AS(RMH_MOCA_VERSION_11,                 0x11) \
    AS(RMH_MOCA_VERSION_20,                 0x20)
typedef enum RMH_MoCAVersion { ENUM_RMH_MoCAVersion } RMH_MoCAVersion;

#define RMH_LOG_NONE 0
#define RMH_LOG_DEFAULT  (RMH_LOG_ERROR | RMH_LOG_WARNING | RMH_LOG_MESSAGE)

#define ENUM_RMH_LogLevel \
    AS(RMH_LOG_ERROR,                       1u << 0) \
    AS(RMH_LOG_WARNING,                     1u << 1) \
    AS(RMH_LOG_MESSAGE,                     1u << 2) \
    AS(RMH_LOG_DEBUG,                       1u << 3) \
    AS(RMH_LOG_TRACE,                       1u << 4)
typedef enum RMH_LogLevel { ENUM_RMH_LogLevel } RMH_LogLevel;

#define ENUM_RMH_Event \
    AS(RMH_EVENT_LINK_STATUS_CHANGED,           1u << 0) \
    AS(RMH_EVENT_MOCA_VERSION_CHANGED,          1u << 1) \
    AS(RMH_EVENT_LOW_BANDWIDTH,                 1u << 2) \
    AS(RMH_EVENT_NODE_JOINED,                   1u << 4) \
    AS(RMH_EVENT_NODE_DROPPED,                  1u << 5) \
    AS(RMH_EVENT_ADMISSION_STATUS_CHANGED,      1u << 6) \
    AS(RMH_EVENT_NC_ID_CHANGED,                 1u << 7) \
    AS(RMH_EVENT_API_PRINT,                     1u << 8) \
    AS(RMH_EVENT_DRIVER_PRINT,                  1u << 9)
typedef enum RMH_Event { ENUM_RMH_Event } RMH_Event;

typedef struct RMH_EventData {
    union {
        struct {
            RMH_LinkStatus status;
        } RMH_EVENT_LINK_STATUS_CHANGED;
        struct {
            RMH_AdmissionStatus status;
        } RMH_EVENT_ADMISSION_STATUS_CHANGED;
        struct {
            RMH_MoCAVersion version;
        } RMH_EVENT_MOCA_VERSION_CHANGED;
        struct {
        } RMH_EVENT_LOW_BANDWIDTH;
        struct {
            bool ncValid;
            uint32_t ncNodeId;
        } RMH_EVENT_NC_ID_CHANGED;
        struct {
            uint32_t nodeId;
        } RMH_EVENT_NODE_JOINED;
        struct {
            uint32_t nodeId;
        } RMH_EVENT_NODE_DROPPED;
        struct {
            RMH_LogLevel logLevel;
            const char *logMsg;
        } RMH_EVENT_API_PRINT;
        struct {
            RMH_LogLevel logLevel;
            const char *logMsg;
        } RMH_EVENT_DRIVER_PRINT;
    };
} RMH_EventData;
typedef void (*RMH_EventCallback)(const enum RMH_Event event, const struct RMH_EventData *eventData, void* userContext);

typedef struct RMH_NodeList_Uint32_t {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    uint32_t nodeValue[RMH_MAX_MOCA_NODES];
} RMH_NodeList_Uint32_t;

typedef struct RMH_NodeList_Mac {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    RMH_MacAddress_t nodeValue[RMH_MAX_MOCA_NODES];
} RMH_NodeList_Mac;

typedef struct RMH_NodeMesh_Uint32_t {
    bool nodePresent[RMH_MAX_MOCA_NODES];
    RMH_NodeList_Uint32_t nodeValue[RMH_MAX_MOCA_NODES];
} RMH_NodeMesh_Uint32_t;

typedef enum RMH_APIParamDirection {
    RMH_INPUT_PARAM,
    RMH_OUTPUT_PARAM,
}RMH_APIParamDirection;

typedef struct RMHGeneric_Param {
    const RMH_APIParamDirection direction;
    const char *name;
    const char *type;
    const char *desc;
} RMHGeneric_Param;

typedef struct RMH_API {
    const char *apiName;
    const bool socApiExpected;
    const bool genericApiExpected;
    const char *socApiName;
    const char *apiDefinition;
    const char *apiDescription;
    const char *tags;
    const uint32_t apiNumParams;
    const RMHGeneric_Param* apiParams;
} RMH_API;

typedef struct RMH_APIList {
    char apiListName[32];
    uint32_t apiListSize;
    RMH_API *apiList[RMH_MAX_NUM_APIS];
} RMH_APIList;

typedef struct RMH_APITagList {
    uint32_t tagListSize;
    RMH_APIList tagList[RMH_MAX_NUM_TAGS];
} RMH_APITagList;

#ifdef __cplusplus
}
#endif

#endif /*RDK_MOCA_HAL_TYPES_H*/