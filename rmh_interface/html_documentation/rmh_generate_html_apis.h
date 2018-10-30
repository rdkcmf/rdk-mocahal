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



#define __COMMAND_MAKE_HTML_OUTPUT(DIR, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)\
    <tr class="apiParameter"> \
        <td> \
            <span class="apiParameterDirection">DIR</span>\
        </td><td> \
            <span class="apiParameterName"> VARIABLE_NAME</span> \
        </td><td> \
            <span class="apiParameterDescription">DESCRIPTION_STR</span> \
        </td> \
    </tr>


#define __COMMAND_MAKE_HTML_RMH_INPUT_PARAM(VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) __COMMAND_MAKE_HTML_OUTPUT(Input, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)
#define __COMMAND_MAKE_HTML_RMH_OUTPUT_PARAM(VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) __COMMAND_MAKE_HTML_OUTPUT(Output, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)
#define __COMMAND_MAKE_HTML(VARIABLE_DIRECTION, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) __COMMAND_MAKE_HTML_##VARIABLE_DIRECTION(VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)
#define RMH_PARAMETERS_TO_HTML(PARAMS_LIST) \
    <table> \
        __EXE_NUM_PARAMS_X(__COMMAND_MAKE_HTML, __GET_ARGS(PARAMS_LIST)) \
    </table>

#define RMH_API_TO_HTML(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR, API_TYPE) \
    <div class="api" id=#API_NAME> \
        <div> \
            <p style="text-align:left;"> \
                <span class="apiTitle">API_NAME</span> \
                API_TYPE \
            </p> \
        </div> \
        <span class="apiSectionHeader">NAME:</span><BR> \
        <span class="apiSectionBody">API_NAME</span><BR> \
        <span class="apiSectionHeader">DECLARATION:</span><BR> \
        <div class="apiDeclaration">_DECLARATION</div><BR> \
        <span class="apiSectionHeader">DESCRIPTION:</span><BR> \
        <div class="apiSectionBody">_DESCRIPTION_STR</div><BR> \
        <span class="apiSectionHeader">PARAMETERS:</span><BR> \
        <div class="apiParameters">RMH_PARAMETERS_TO_HTML(PARAMS_LIST)</div><BR> \
        <span class="apiSectionHeader">TAGS:</span><BR> \
        <span class="apiSectionBody">TAGS_STR</span><BR> \
        <BR><a href="#top">Back to top</a> \
    </div>


#define RMH_API_IMPLEMENTATION_SOC_ONLY(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
             RMH_API_TO_HTML(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR, <a class="apiType" href="#toc_soc">SoC Implemented API</a>)

#define RMH_API_IMPLEMENTATION_GENERIC_ONLY(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
             RMH_API_TO_HTML(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR, <a class="apiType" href="#toc_generic">Helper API</a>)

#define RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
             RMH_API_TO_HTML(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR, <a class="apiType" href="#toc_soc">SoC Implemented API</a>)

#define RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR) \
             RMH_API_TO_HTML(_DECLARATION, API_NAME, _DESCRIPTION_STR, PARAMS_LIST, WRAP_API, TAGS_STR, <a class="apiType" href="#toc_soc">SoC Implemented API</a>)


/* Reinclude API header to print the APIs */
#undef RMH_API_H
#include "rmh_api.h"
