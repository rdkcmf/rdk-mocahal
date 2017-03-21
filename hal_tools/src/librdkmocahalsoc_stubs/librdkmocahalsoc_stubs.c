#include <stdio.h>

#include "rdk_moca_hal_types.h"

RMH_Handle RMH_SOC_STUBS_INIT_UNIMPLEMENTED(void) {return NULL;}
#define __RMH_INIT_ATTRIB__ __attribute__ ((weak, alias ("RMH_SOC_STUBS_INIT_UNIMPLEMENTED")))

void RMH_SOC_STUBS_DEST_UNIMPLEMENTED(void) {}
#define __RMH_DEST_ATTRIB__ __attribute__ ((weak, alias ("RMH_SOC_STUBS_DEST_UNIMPLEMENTED")))

RMH_Result RMH_SOC_STUBS_API_UNIMPLEMENTED(void) {return RMH_UNIMPLEMENTED;}
#define __RMH_API_ATTRIB__ __attribute__ ((weak, alias ("RMH_SOC_STUBS_API_UNIMPLEMENTED")))

#include "rdk_moca_hal_soc.h"


