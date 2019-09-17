/* Includes ------------------------------------------------------------------*/

#include "include.h"
#include "nx1_dac.h"
#include "nx1_adc.h"

#if _AUDIO_MODULE==1
#if _AUDIO_MULTI_CHANNEL
extern S32 DacMixBuf[];
#else
extern U16 DacMixBuf[];
#endif
//------------------------------------------------------------------//
// Init Dac
// Parameter: 
//          chx         : DAC_CH0, DAC_CH1
//          sample_rate : sample rate
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_MIDI_InitDac(CH_TypeDef *chx,U16 sample_rate)
{    
    InitDac(chx,sample_rate);
}    
//------------------------------------------------------------------//
// Dac command
// Parameter: 
//          chx         : DAC_CH0, DAC_CH1
//          cmd         : ENABLE,DISABLE
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_MIDI_DacCmd(CH_TypeDef *chx,U8 cmd)
{    
    DAC_Cmd(chx,cmd);
}    
//------------------------------------------------------------------//
// Write Dac Fifo
// Parameter: 
//          size: size to write
//          psrc16: source pointer
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_MIDI_WriteDac(U8 size,U16* psrc16)
{    
    U8 i;

    if(psrc16!=0)
    {
        for(i=0;i<size;i++)
        {
            DacMixBuf[i]+=(S16)*psrc16;
            psrc16++;
        }   
    }   
}   



void CB_MIDI_PlayStart(void)
{
	
}  
void CB_MIDI_PlayEnd(void)
{
	
}          

#endif
