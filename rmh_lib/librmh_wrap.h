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

#ifndef LIB_RMH_API_WRAP_H
#define LIB_RMH_API_WRAP_H

/**********************************************************************************************************************
This API wrap uses a bit of preprocessor abuse but in the end I think it's worth it. These macros allow us to:

1. Wrap every RMH API to allow for
    A. Dynamically check SoC library for existance of the API and return UNIMPLEMENTED if it's not found
    B. Enter/Exit/Return code logging
    C. Timing APIs
2. Generate a struct containing all APIs so when a new one is added it need only be added to the header file and it
   will automatically appears in the rmh test app
3. Time the API to discover where it might be running slow
**********************************************************************************************************************/



/**********************************************************************************************************************
These are the basic parameter macros.
    1) PARAMETERS() just returns everything it was passed. It's used for grouping only.'
    2) INPUT_PARAM() and OUTPUT_PARAM() simply add RMH_INPUT_PARAM and RMH_OUTPUT_PARAM, respectively, to the parameter
       list.
    3) __GET_ARGS() is stripping outter parentheses via __GET_ARGS_UNWRAP
**********************************************************************************************************************/
#define __GET_ARGS_UNWRAP(...) __VA_ARGS__
#define __GET_ARGS(X) __GET_ARGS_UNWRAP X
#define INPUT_PARAM(VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) (RMH_INPUT_PARAM, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)
#define OUTPUT_PARAM(VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) (RMH_OUTPUT_PARAM, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR)
#define PARAMETERS(...) (__VA_ARGS__)


/**********************************************************************************************************************
This group of macros operates on the parameter list of the API.

The entry point to this set of macros is __EXE_NUM_PARAMS_X(). It takes one of the COMMAND macros as the first
argument. The remaining arguments are the list of parameters for the API in the format:

    (direction, name, type, "description")

For example, you might call:

    __EXE_NUM_PARAMS_X(__COMMAND_MAKE_VARIABLE_LIST, \
                    (RMH_INPUT_PARAM, handle, const RMH_Handle, "The RMH handle"), \
                    (RMH_OUTPUT_PARAM, value, uint32_t*, "My output"))

The first thing __EXE_NUM_PARAMS_X() will do is determine the number of arguments it has been passed (excluding the
command). It uses __GET_EXE_MACRO() to do this. In this cases the number of arguments is two.  It then calls COMMAND()
for each parameter. In our example this would be:

    __COMMAND_MAKE_VARIABLE_LIST(RMH_INPUT_PARAM, handle, const RMH_Handle, "The RMH handle")
    __COMMAND_MAKE_VARIABLE_LIST(RMH_OUTPUT_PARAM, value, uint32_t*, "My output")

Finally, the output of each COMMAND() is placed in a comma seperated list. This is what will be returned as the final
result of __EXE_NUM_PARAMS_X(). In our example, the final output would be:

    handle, value
**********************************************************************************************************************/
#define __COMMAND_MAKE_VARIABLE_LIST(VARIABLE_DIRECTION, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) VARIABLE_NAME                                                           /* Goal is to create a comma seperated list of all parameter variables */
#define __COMMAND_MAKE_API_STRUCT(VARIABLE_DIRECTION, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) { VARIABLE_DIRECTION, #VARIABLE_NAME, #VARIABLE_TYPE, DESCRIPTION_STR }    /* Goal is to create structure entry which describes this parameter */
#define __COMMAND_MAKE_TYPE_LIST(VARIABLE_DIRECTION, VARIABLE_NAME, VARIABLE_TYPE, DESCRIPTION_STR) VARIABLE_TYPE VARIABLE_NAME                                                 /* Goal is to create a comma seperated list of all variables and types */

#define __EXE_NUM_PARAMS_0(COMMAND)
#define __EXE_NUM_PARAMS_1(COMMAND, PARAM)      COMMAND PARAM
#define __EXE_NUM_PARAMS_2(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_1(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_3(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_2(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_4(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_3(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_5(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_4(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_6(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_5(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_7(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_6(COMMAND, __VA_ARGS__)
#define __EXE_NUM_PARAMS_8(COMMAND, PARAM, ...) COMMAND PARAM, __EXE_NUM_PARAMS_7(COMMAND, __VA_ARGS__)
#define __GET_EXE_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define __EXE_NUM_PARAMS_X(COMMAND, ...) __GET_EXE_MACRO(__VA_ARGS__, __EXE_NUM_PARAMS_8, __EXE_NUM_PARAMS_7, __EXE_NUM_PARAMS_6, __EXE_NUM_PARAMS_5, __EXE_NUM_PARAMS_4, __EXE_NUM_PARAMS_3, __EXE_NUM_PARAMS_2, __EXE_NUM_PARAMS_1)(COMMAND, __VA_ARGS__)


/**********************************************************************************************************************
This macro "wraps" an API. It will:
1. Define a function specific to wrapping this API
2. The while(0) code is dead code and simply a compiler check to verify all the parameters have been added to the API
   DECLARATION in the header file. Note, there is no checking of 'type' here. It could be done but it gets much more
   complicated and probably not worth the complexity.
3. The function will execute the generic and SoC versions of the API in order of depending on the values of
   GENERIC_API_NAME, SOC_API_ENABLED, and SOC_BEFORE_GENERIC. If an api fails, further api calls will not be made.
4. We use pRMH_APIWRAP_PreAPIExecute(), pRMH_APIWRAP_GetSoCAPI() and pRMH_APIWRAP_PostAPIExecute() to do as much basic
   possibile outside of the macro. The param lists prevent us from doing everything in functions.
******************************************/
#define __RMH_API_WRAP_TRUE(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, GENERIC_ENABLED, SOC_ENABLED, SOC_BEFORE_GENERIC) \
    RMH_Result GENERIC_IMPL__##API_NAME (__EXE_NUM_PARAMS_X(__COMMAND_MAKE_TYPE_LIST, __GET_ARGS(PARAMS_LIST))); \
    RMH_Result SoC_IMPL__##API_NAME (__EXE_NUM_PARAMS_X(__COMMAND_MAKE_TYPE_LIST, __GET_ARGS(PARAMS_LIST))); \
    DECLARATION { \
        RMH_Result ret = RMH_SUCCESS; \
        RMH_Result (*socAPI)() = NULL; \
        while(0) { API_NAME( __EXE_NUM_PARAMS_X(__COMMAND_MAKE_VARIABLE_LIST, __GET_ARGS(PARAMS_LIST)) ); } \
        pRMH_APIWRAP_PreAPIExecute(handle, #API_NAME, SOC_ENABLED, SOC_BEFORE_GENERIC); \
        if (SOC_ENABLED) { \
            socAPI = pRMH_APIWRAP_GetSoCAPI(handle, "SoC_IMPL__"#API_NAME); \
        } \
        if (ret == RMH_SUCCESS && SOC_ENABLED && SOC_BEFORE_GENERIC) { \
            ret = (socAPI) ? socAPI(handle-> __EXE_NUM_PARAMS_X(__COMMAND_MAKE_VARIABLE_LIST, __GET_ARGS(PARAMS_LIST))) : RMH_UNIMPLEMENTED; \
        } \
        if (GENERIC_ENABLED && ret == RMH_SUCCESS) { \
            ret = (!SOC_ENABLED || socAPI) ? GENERIC_IMPL__##API_NAME(__EXE_NUM_PARAMS_X(__COMMAND_MAKE_VARIABLE_LIST, __GET_ARGS(PARAMS_LIST))) : RMH_UNIMPLEMENTED; \
        } \
        if (ret == RMH_SUCCESS && SOC_ENABLED && !SOC_BEFORE_GENERIC) { \
            ret = (socAPI) ? socAPI(handle-> __EXE_NUM_PARAMS_X(__COMMAND_MAKE_VARIABLE_LIST, __GET_ARGS(PARAMS_LIST))) : RMH_UNIMPLEMENTED; \
        } \
        return pRMH_APIWRAP_PostAPIExecute(handle, #API_NAME, ret); \
    } \
    __RMH_REGISTER_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, GENERIC_ENABLED, SOC_ENABLED, SOC_BEFORE_GENERIC, SoC_IMPL__##API_NAME, GENERIC_IMPL__##API_NAME);

#define __RMH_API_WRAP_FALSE(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, GENERIC_ENABLED, SOC_ENABLED, SOC_BEFORE_GENERIC) \
    __RMH_REGISTER_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, GENERIC_ENABLED, false, SOC_BEFORE_GENERIC,  SoC_IMPL__##API_NAME, GENERIC_IMPL__##API_NAME);


#define __RM_DEFINE_API_WRAP_TRUE(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, DECLARE_NAME) \
    RMH_Result DECLARE_NAME (__EXE_NUM_PARAMS_X(__COMMAND_MAKE_TYPE_LIST, __GET_ARGS(PARAMS_LIST)));

#define __RM_DEFINE_API_WRAP_FALSE(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, DECLARE_NAME) \
    DECLARATION;


/**********************************************************************************************************************
This macro "registers" an API which allows anyone who links this library to get a list of APIs and descriptions.

1. Create a structure describing the parameters. It uses __EXE_NUM_PARAMS_X() macro to construct this. This is
   pRMH_PARAMS_##API_NAME
2. Create a structure describing the API itself. This is pRMH_API_##API_NAME
3. Register the API in the global context by creating a constructor function. This is
   pRMH_APIWRAP_RegisterAPI_##API_NAME
**********************************************************************************************************************/
#define __RMH_REGISTER_API(DECLARATION, API_NAME, DESCRIPTION_STR, PARAMS_LIST, TAGS_STR, GENERIC_ENABLED, SOC_ENABLED, SOC_BEFORE_GENERIC, SOC_API_NAME, GEN_API_NAME) \
    static RMHGeneric_Param pRMH_PARAMS_##API_NAME[] = { __EXE_NUM_PARAMS_X(__COMMAND_MAKE_API_STRUCT, __GET_ARGS(PARAMS_LIST)) }; \
    static RMH_API pRMH_API_##API_NAME = { #API_NAME, SOC_ENABLED, GENERIC_ENABLED, #SOC_API_NAME, #DECLARATION, DESCRIPTION_STR, TAGS_STR, sizeof(pRMH_PARAMS_##API_NAME)/sizeof(pRMH_PARAMS_##API_NAME[0]), pRMH_PARAMS_##API_NAME }; \
    __attribute__((constructor(300))) static void pRMH_APIWRAP_RegisterAPI_##API_NAME() { pRMH_APIWRAP_RegisterAPI(&pRMH_API_##API_NAME); } \

#endif /* LIB_RMH_API_WRAP_H */
