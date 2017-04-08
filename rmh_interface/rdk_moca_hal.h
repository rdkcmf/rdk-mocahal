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

/***************************************************************************************************************************
RMH_API_IMPLEMENTATION_SOC_ONLY:

  Use this macro to define an RMH API which is only implemented in the SoC RMH library

  Notes:
    1) The SoC library shuld implement the fuction DECLARATION. The function name must be identical to DECLARATION
    2) If no API is provided by the SoC libaray RMH_UNIMPLEMENTED will be returned
***************************************************************************************************************************/
#ifndef RMH_API_IMPLEMENTATION_SOC_ONLY
#define RMH_API_IMPLEMENTATION_SOC_ONLY(DECLARATION, ...) DECLARATION;
#endif


/***************************************************************************************************************************
RMH_API_IMPLEMENTATION_GENERIC_ONLY:

  Use this macro to define an RMH API which is only implemented in the generic RMH library

  Notes:
    1) The generic library must implement the fuction described by DECLARATION. The function name must be prefixed by 'GENERIC_IMPL__'
    2) No calls to the SoC library will be made
***************************************************************************************************************************/
#ifndef RMH_API_IMPLEMENTATION_GENERIC_ONLY
#define RMH_API_IMPLEMENTATION_GENERIC_ONLY(DECLARATION, ...) DECLARATION;
#endif


/***************************************************************************************************************************
RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC:

  Use this macro to define an RMH API which is implemented in both the generic and the SoC RMH libraries. The generic API
  will be called first.

  Notes:
    1) The SoC API is called only if the gneric function succeeds
    2) The generic library must implement the fuction described by DECLARATION. The function name must be prefixed by 'GENERIC_IMPL__'
    3) The SoC library shuld implement the fuction DECLARATION. The function name must be identical to DECLARATION
    4) If no SoC function is defined no call to the SoC library is made and the result of the generic function is returned
***************************************************************************************************************************/
#ifndef RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC
#define RMH_API_IMPLEMENTATION_GENERIC_THEN_SOC(DECLARATION, ...) DECLARATION;
#endif


/***************************************************************************************************************************
RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC:

  Use this macro to define an RMH API which is implemented in both the generic and the SoC RMH libraries. The SoC API
  will be called first.

  Notes:
    1) The generic API is called only if the SoC function exists and succeeds
    2) The generic library must implement the fuction described by DECLARATION. The function name must be prefixed by 'GENERIC_IMPL__'
    3) The SoC library shuld implement the fuction DECLARATION. The function name must be identical to DECLARATION
    4) When the generic API is called, the parameters will be preloaded with the output of the SoC version of the API
***************************************************************************************************************************/
#ifndef RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC
#define RMH_API_IMPLEMENTATION_SOC_THEN_GENERIC(DECLARATION, ...) DECLARATION;
#endif


/***************************************************************************************************************************
RMH_API_IMPLEMENTATION_NO_WRAP:

  Use this macro to define an RMH API which will not be wrapped or automatically executed in any way.

  Notes:
    1) The generic library shuld implement the fuction DECLARATION. The function name must be identical to DECLARATION
    2) The generic fuction implementation will manually call the SoC libary if it so chooses
    3) The generic fuction will decide how and what the return value will be
***************************************************************************************************************************/
#ifndef RMH_API_IMPLEMENTATION_NO_WRAP
#define RMH_API_IMPLEMENTATION_NO_WRAP(DECLARATION, ...) DECLARATION;
#endif


#include "rmh_type.h"
#include "rmh_api.h"

#endif /*RDK_MOCA_HAL_H*/
