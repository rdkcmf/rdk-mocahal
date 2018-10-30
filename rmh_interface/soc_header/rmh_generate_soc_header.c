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
#include "librmh_wrap.h"

#define RMH_API_IMPLEMENTATION_SOC_ONLY(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RM_DEFINE_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, SoC_IMPL__##API_NAME)

#define RMH_API_IMPLEMENTATION_GENERIC_ONLY(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR)

#define RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RM_DEFINE_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, SoC_IMPL__##API_NAME)

#define RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
    __RM_DEFINE_API_WRAP_##WRAP_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, SoC_IMPL__##API_NAME)

/* Reinclude API header to use the redefined macros to setup necessary functions and structs */
#undef RMH_API_H
#include "rmh_api.h"
