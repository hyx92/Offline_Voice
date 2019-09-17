#include "include.h"
#include "utility_define.h"

#if _SPI_MODULE
const U16 XIP_VAD_ParametersTable[] _XIP_RODATA = {VAD_THRESHOLD, VAD_ACTIVE_COUNT, VAD_MUTE_COUNT};

U16 XIP_VAD_Parameters(U8 index)
{
	return XIP_VAD_ParametersTable[index];
}
#endif