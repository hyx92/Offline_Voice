/* Includes ------------------------------------------------------------------*/
#include "include.h"
#include "nx1_lib.h"


//------------------------------------------------------------------//
// Utility service loop
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//
void UTILITY_ServiceLoop(void)
{

#if _AUDIO_SENTENCE
	Sentence_ServiceLoop();
#endif

#if _ACTION_MODULE
	ACTION_ServiceLoop();
#endif

#if	_QFID_MODULE
	QFID_ServiceLoop();
#endif

#if _TOUCHKEY_MODULE
	TOUCH_ServiceLoop();
#endif

#if _ADPCM_RECORD_SOUND_TRIG
    ADPCM_RecordSoundTrigServiceLoop();
#endif

}
