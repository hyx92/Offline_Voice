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
void CB_CELP_InitDac(CH_TypeDef *chx,U16 sample_rate)
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
void CB_CELP_DacCmd(CH_TypeDef *chx,U8 cmd)
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
void CB_CELP_WriteDac(U8 size,U16* psrc16)
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
//------------------------------------------------------------------//
// Change dac sample rate
// Parameter:
//          chx         : DAC_CH0, DAC_CH1
//          sample_rate : sample rate
// return value:
//          NONE
//------------------------------------------------------------------//
void CB_CELP_ChangeSR(CH_TypeDef *chx,U16 sample_rate)
{ 
    SetSampleRate(chx,sample_rate);
}


void CB_CELP_PlayStart(void)
{
	
}  
void CB_CELP_PlayEnd(void)
{
	
}          

#endif
