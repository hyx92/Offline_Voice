
//============================================
#include "nx1_lib.h"
#include "nx1_gpio.h"
#include "include.h"
#include "nx1_config.h"
#include "TouchTab.h"

#if _TOUCHKEY_MODULE
volatile STouchPAD TouchPAD[C_TouchMaxPAD];
//------------------------------------------------------------------//
// Touch Initial CB
// Parameter: 
//			NONE
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_TouchInit()
{
	Touch_SleepTime(C_TouchWakeTime);				//	Initial Touch Wake Down Count Time.
	Touch_SetWakeUpMaxPAD(C_TouchWakeUpMaxPAD);		//	Initial Touch WakeUp PAD.
#ifdef	C_PAD0_IN
		Touch_SetSensitivityLevel(0,TOUCH_SENSITIVE_LV0);	// PAD0  
#endif
#ifdef	C_PAD1_IN
		Touch_SetSensitivityLevel(1,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD2_IN
		Touch_SetSensitivityLevel(2,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD3_IN
		Touch_SetSensitivityLevel(3,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD4_IN
		Touch_SetSensitivityLevel(4,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD5_IN
		Touch_SetSensitivityLevel(5,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD6_IN
		Touch_SetSensitivityLevel(6,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD7_IN
		Touch_SetSensitivityLevel(7,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD8_IN
		Touch_SetSensitivityLevel(8,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD9_IN
		Touch_SetSensitivityLevel(9,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD10_IN
		Touch_SetSensitivityLevel(10,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD11_IN
		Touch_SetSensitivityLevel(11,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD12_IN
		Touch_SetSensitivityLevel(12,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD13_IN
		Touch_SetSensitivityLevel(13,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD14_IN
		Touch_SetSensitivityLevel(14,TOUCH_SENSITIVE_LV0);
#endif
#ifdef	C_PAD15_IN
		Touch_SetSensitivityLevel(15,TOUCH_SENSITIVE_LV0);
#endif
}

//------------------------------------------------------------------//
// Touch Press CB
// Parameter: 
//			u8KeyValue:TOUCH_PAD0 ~ TOUCH_PAD15
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_TouchPress(U8 u8KeyValue)
{
	Touch_SetEnforceTime(C_TouchEnforceTime);		// Set Enforce Calibration Downcount time = C_TouchEnforceTime

}

//------------------------------------------------------------------//
// Touch Release CB
// Parameter: 
//			u8KeyValue:TOUCH_PAD0 ~ TOUCH_PAD15
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_TouchRelease(U8 u8KeyValue)
{
	Touch_SleepTime(C_TouchWakeTime);

}

//------------------------------------------------------------------//
// Touch Enforce Calibration CB
// Parameter: 
//			u8PADCount:TOUCH_PAD0 ~ TOUCH_PAD15
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_EnforceCalibration(U8 u8PADCount)
{
	CB_TouchRelease(u8PADCount);

}

//------------------------------------------------------------------//
// Touch Calibration CB
// Parameter: 
//			u8PADCount:TOUCH_PAD0 ~ TOUCH_PAD15
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_Calibration(U8 u8PADCount)
{

}

//------------------------------------------------------------------//
// Touch StandByScan
// Parameter: 
//			NONE
// return value:
//          None
//------------------------------------------------------------------// 
void CB_StandByScan()
{
	if(0)//User defined condition
	{
		Touch_SleepTime(C_TouchWakeTime);
		Touch_SetEnforceWakeup();
	}
}
//===================================================
#endif