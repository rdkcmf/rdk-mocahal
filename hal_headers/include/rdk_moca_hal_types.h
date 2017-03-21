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

#define RMH_MAX_MOCA_NODES 16

typedef struct RMH* RMH_Handle;

#define ENUM_RMH_Result \
    AS(RMH_SUCCESS, 0)\
    AS(RMH_FAILURE, 1)\
    AS(RMH_INVALID_PARAM, 2)\
    AS(RMH_INVALID_NETWORK_STATE, 3)\
    AS(RMH_INVALID_INTERNAL_STATE, 4)\
    AS(RMH_INVALID_ID, 5)\
    AS(RMH_TIMEOUT, 6)\
    AS(RMH_INSUFFICIENT_SPACE, 7)\
    AS(RMH_NOT_SUPPORTED, 8)\
    AS(RMH_UNIMPLEMENTED, 9)

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
    AS(RMH_LOG_LEVEL_NONE, 0)\
    AS(RMH_LOG_LEVEL_ERROR, 1)\
    AS(RMH_LOG_LEVEL_DEFAULT, 2)\
    AS(RMH_LOG_LEVEL_DEBUG, 3)\
    AS(RMH_LOG_LEVEL_FULL, 4)

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
static inline const char * const RMH_ResultToString(const RMH_Result value) { return RMH_ResultStr[value]; }

const char * const RMH_LinkStatusStr[] = { ENUM_RMH_LinkStatus };
static inline const char * const RMH_LinkStatusToString(const RMH_LinkStatus value) { return RMH_LinkStatusStr[value]; }

const char * const RMH_PowerModeStr[] = { ENUM_RMH_PowerMode };
static inline const char * const RMH_PowerModeToString(const RMH_PowerMode value) { return RMH_PowerModeStr[value]; }

const char * const RMH_LogLevelStr[] = { ENUM_RMH_LogLevel };
static inline const char * const RMH_LogLevelToString(const RMH_LogLevel value) { return RMH_LogLevelStr[value]; }

const char * const RMH_MoCAVersionStr[] = { ENUM_RMH_MoCAVersion };
static inline const char * const RMH_MoCAVersionToString(const RMH_MoCAVersion value) {
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

typedef uint8_t RMH_MacAddress_t[6];

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


#endif /*RDK_MOCA_HAL_TYPES_H*/