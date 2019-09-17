#include "include.h"
#include "nx1_adc.h"
#include "nx1_gpio.h"
#include <string.h>
#include "nx1_lib.h"
#include "NX1_reg.h"
#include "nx1_smu.h"
#include "utility_define.h"
#include "debug.h"

#if _VR_VAD || _VR_VAD_TIMEOUT_EXTENSION

#define VAD_SR		16000

// VAD Function Select
#define VAD_FUNC_SELECT         CMP_AVG
#define CMP_AVG					1
#define CMP_DIRECT				2
#define CMP_CAL					3

#define DC_REMOVE				0

#define ABS(a) (((a) < 0) ? -(a) : (a))

#if DC_REMOVE
static const S16 DCRemove_10kHz[13] = {
	16384,  //a0-0 
	-15938, //a1-0 
	0,  //a2-0 
	16161,  //b0-0 
	-16161, //b1-0 
	0,  //b2-0 
	16384,  //a0-1 
	-32309, //a1-1 
	15938,  //a2-1 
	16158,  //b0-1 
	-32316, //b1-1 
	16158,  //b2-1 
	14,     //shift scale
};
#endif

#if VAD_FUNC_SELECT==CMP_DIRECT
static volatile struct{
    U16 average;
    U8  state;              
    U8  count1;
    U8  count2;
    U8  flag;            
}VadCtl;
#endif


#if (VAD_FUNC_SELECT==CMP_AVG) || (VAD_FUNC_SELECT==CMP_CAL)
static volatile struct{
    U16 average;
    U8  state;              
    U8  count;
    U8  flag;            
	U8	flag2;
}VadCtl;
#endif

#if DC_REMOVE
static S32 RunFilterBuf[8];
#endif

//------------------------------------------------------------------//
// Voice activity detection initial
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------// 
void VAD_Adc_Init(void)
{	
#if  1
	InitAdc(VAD_SR);
	ADC_Cmd(ENABLE);

#else
//	// ADC init
	P_SMU->FUNC_Ctrl |= C_Func_TMR_En;
   	P_TIMER2->Data = 999;
   	P_TIMER2->Ctrl = C_TMR_CLK_Src_HiCLK | C_TMR_CLK_Div_1 | C_TMR_Auto_Reload_En;
//   	P_SMU->FUNC_Ctrl |= C_Func_ADC_En;
   	P_ADC->FIFO_Ctrl &= ~C_ADC_Data_Store_FIFO;
   	P_ADC->Ctrl &= ~C_ADC_Sample_Cycle; 
   	P_ADC->Ctrl |= C_ADC_CLK_Div_8 | C_ADC_Sample_Cycle_8 | C_ADC_Trig_Timer2 | C_ADC_En | C_ADC_CH0 | C_ADC_CH0_From_MIC; 
    P_TIMER2->Ctrl |= C_TMR_En;
    TIMER_IntCmd(TIMER2,ENABLE);
    ADC_SetGain(31);
    ADC_MicPreGain(C_ADC_MIC_AMP_Gain_30X);
#endif

}


//------------------------------------------------------------------//
// Voice activity detection process
// Parameter: 
//          audio_buf: audio buffer
//          samples: samples to process
// return value:
//          NONE
//------------------------------------------------------------------//     
void VadProcess(S16 *audio_buf,U16 samples)
{
    U16 i;
    U32 sum=0;
   
//VAD level function select
//Compare directly 	
#if VAD_FUNC_SELECT==CMP_DIRECT
	U16 adc_v[samples];

    for(i=0;i<samples;i++)
    {
        adc_v[i]=ABS(audio_buf[i]);
        
//        dprintf("adc_v[%d]=%d\r\n",i,adc_v[i]);
        
        if (adc_v[i]>VadCtl.average)
    	{
  			if(VadCtl.count1>=VAD_ACTIVE_COUNT)
  			{
    			VadCtl.flag=1;
    			VadCtl.count1=0;
    			VadCtl.count2=0;
    			i=samples;
//    			dprintf("VadCtl.flag=%x\r\n",VadCtl.flag);
        	}
        	else
        	{
        		VadCtl.count1++;
//        		dprintf("[>]adc_v=%d\r\n",adc_v[i]);
    		}
    	}
    	else
    	{

	  		if(VadCtl.count2>=VAD_MUTE_COUNT)
	  		{
    			VadCtl.flag=0;
    			VadCtl.count1=0;
    			VadCtl.count2=0;
    			i=samples;
//    			dprintf("VadCtl.flag=%x\r\n",VadCtl.flag);
       	 	}
        	else
        	{
        		VadCtl.count2++;
//        		dprintf("[<]adc_v=%d\r\n",adc_v[i]);
    		}
    	}
            
    }    
#endif 
    
//VAD level function select
//Average compare 	
#if VAD_FUNC_SELECT==CMP_AVG

    for(i=0;i<samples;i++){
#if DC_REMOVE
		RunFilter(&audio_buf[i], RunFilterBuf, DCRemove_10kHz);
#endif
        sum=sum+ABS(audio_buf[i]);
    }    
    sum=sum/samples;
	//dprintf("VAD sum = %d\r\n",sum);
    //blake test start
    if (sum>VadCtl.average)
    {

	#if _SPI_MODULE
		if(VadCtl.count>=XIP_VAD_Parameters(1))
	#else 
  		if(VadCtl.count>=VAD_ACTIVE_COUNT)
	#endif
  		{
    		VadCtl.flag=1;
			VadCtl.flag2=1;
    		VadCtl.count=0;
        }
        else
        {
        	VadCtl.count++;
    	}
    }
    else
    {

	#if _SPI_MODULE
		if(VadCtl.count>=XIP_VAD_Parameters(2))
	#else 
	  	if(VadCtl.count>=VAD_MUTE_COUNT)
	#endif
	  	{
    		//VadCtl.flag=0;
    		VadCtl.count=0;
        }
        else
        {
        	VadCtl.count++;
    	}
    }
#endif

//Average caculate compare	
#if VAD_FUNC_SELECT==CMP_CAL
   	for(i=0;i<samples;i++){
        sum=sum+ABS(audio_buf[i]);
    }    
    sum=sum/samples;
    
    VadCtl.average=VadCtl.average*0.9+sum*0.1;  
    if(VadCtl.state==0){  
        if(sum>VadCtl.average){
            sum=sum-VadCtl.average;
        }else{
            sum=0;
        }        
        if(sum>VAD_HI_THRESHOLD){
            VadCtl.count++;    
        }else{
            VadCtl.count=0;
        }    
        if(VadCtl.count>=VAD_ACTIVE_COUNT){
            VadCtl.state=1;    
            VadCtl.count=0;
        }
    }else{
         if(sum<VadCtl.average){
             VadCtl.count++;    
        }else{
            sum=sum-VadCtl.average;
            if(sum>VAD_HI_THRESHOLD){
                VadCtl.count=0;    
            }else{
                VadCtl.count++;    
            }  
        }        
        
        if(VadCtl.count>=VAD_MUTE_COUNT){
            VadCtl.state=0;    
            VadCtl.count=0;
            VadCtl.flag=1;           
        }     

    }  
#endif



}    
//------------------------------------------------------------------//
// Voice activity detection int
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//     
void VadInit(void)
{
#if DC_REMOVE
	U8 i;
	for(i=0;i<8;i++)
	{
		RunFilterBuf[i]=0;
	}
#endif
    memset((void *)&VadCtl, 0, sizeof(VadCtl));   
#if _SPI_MODULE
	VadCtl.average = XIP_VAD_Parameters(0);
#else 
    VadCtl.average = VAD_THRESHOLD;
#endif
}   
//------------------------------------------------------------------//
// Get VAD status
// Parameter: 
//          NONE
// return value:
//          1: voice activity
//          0: no voice activity
//------------------------------------------------------------------//     
U8 VR_GetVadSts(void)
{
    if(VadCtl.flag){
//    	dprintf("1.VR_GetVadSts_VadCtl.flag=%x\r\n",VadCtl.flag);
        VadCtl.flag=0; //blake
        return 1;
    }else{
//    	dprintf("0.VR_GetVadSts_VadCtl.flag=%x\r\n",VadCtl.flag);
        return 0;
    }    
}   
//------------------------------------------------------------------//
// Get VAD status2 - ONLY for VR_ServiceLoop()
// Parameter: 
//          NONE
// return value:
//          1: voice activity
//          0: no voice activity
//------------------------------------------------------------------//     
U8 VR_GetVadSts2(void)
{
    if(VadCtl.flag2){
        VadCtl.flag2=0;
        return 1;
    }else{
        return 0;
    }    
}     
//------------------------------------------------------------------//
// Set VAD Threshold
// Parameter: 
//          threshold: vad threshold value
// return value:
//          NONE
//------------------------------------------------------------------//  
void VadSetThreshold(U16 threshold)
{  
    VadCtl.average = threshold;
}   
#endif