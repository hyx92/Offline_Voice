#include "include.h"


#if _XIP_COC && _VR_STORAGE_TYPE
U16 COC_VR_RamSize(void)
{
	return _VR_RAM_SIZE;
}

U16 COC_VR_EngineRamSize(void)
{
	return _VR_ENGINE_RAM_SIZE;
}

U32 COC_VR_TimeOut(void)
{
	return _VR_TIMEOUT;
}
#endif