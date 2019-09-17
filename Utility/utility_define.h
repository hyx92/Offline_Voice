/* Includes ------------------------------------------------------------------*/
#include "include.h"


// VR VAD setting for user
#define VAD_THRESHOLD			4000	// default value 4000 is for NX1_FDB Ver.B
#define VAD_ACTIVE_TIME		    480		// unit: ms
#define VAD_MUTE_TIME      		580		// unit: ms

// Sound Detection setting for user
#define SD_HIGH_THRESHOLD		1100	// default value 1100 is for NX1_FDB Ver.B
#define SD_LOW_THRESHOLD		900		// default value 900  is for NX1_FDB Ver.B
#define SD_ACTIVE_TIME			80		// unit: ms
#define SD_MUTE_TIME			800		// unit: ms
#define SD_TIMEOUT_TIME			10000	// unit: ms

// VR setting for Unknown Command Detection
#define VR_ID_HOLD_TIME			1000	// unit: ms
#define VR_DELAY_SD_MAX_TIME	300		// unit: ms


/* Internal define -----------------------------------------------------------*/
#if _VR_STORAGE_TYPE
#define VAD_ACTIVE_COUNT    	(VAD_ACTIVE_TIME/16)      
#define VAD_MUTE_COUNT     		(VAD_MUTE_TIME/16)  
#else
#define VAD_ACTIVE_COUNT    	(VAD_ACTIVE_TIME/10)      
#define VAD_MUTE_COUNT     		(VAD_MUTE_TIME/10)
#endif

#define SD_ACTIVE_COUNT			(SD_ACTIVE_TIME/16)
#define SD_MUTE_COUNT			(SD_MUTE_TIME/16)
#define SD_TIMEOUT_COUNT		(SD_TIMEOUT_TIME/16)
