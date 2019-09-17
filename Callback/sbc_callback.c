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
void CB_SBC_InitDac(CH_TypeDef *chx,U16 sample_rate)
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
void CB_SBC_DacCmd(CH_TypeDef *chx,U8 cmd)
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
void CB_SBC_WriteDac(U8 size,U16* psrc16)
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
// Init ADC
// Parameter: 
//  sample_rate:sample rate
// return value:
//          NONE
//------------------------------------------------------------------//
void CB_SBC_InitAdc(U16 sample_rate)
{
    InitAdc(sample_rate);
}   
//------------------------------------------------------------------//
// ADC command
// Parameter: 
//          cmd:ENABLE,DISABLE
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_SBC_AdcCmd(U8 cmd)
{    
#if	defined(P_ADC)
    ADC_Cmd(cmd);
#endif	//#if	defined(P_ADC)
}     
//------------------------------------------------------------------//
// Change dac sample rate
// Parameter:
//          chx         : DAC_CH0, DAC_CH1
//          sample_rate : sample rate
// return value:
//          NONE
//------------------------------------------------------------------//
void CB_SBC_ChangeSR(CH_TypeDef *chx,U16 sample_rate)
{ 
    SetSampleRate(chx,sample_rate);
}


void CB_SBC_PlayStart(void)
{
	
}  
void CB_SBC_PlayEnd(void)
{
	
}       
void CB_SBC_RecStart(void)
{
	
}  
void CB_SBC_RecEnd(void)
{
	
}       

#endif
