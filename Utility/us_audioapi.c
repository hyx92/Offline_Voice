/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "include.h"
#include "nx1_dac.h"
#include "nx1_gpio.h"

#include "nx1_timer.h"
#include "include.h"

#include "nx1_wdt.h"


#if _US_PLAY


////////FOR Ultrasound founction param usapi start  function************
//               17250  17625   18000  18375  18750
//                 1     2       3      4      5     
//				138800 141600  145200 148000 151200 
//tone_conunt     231    226    221    217    212
//                              
//updample        57     56     54     54     52



 const U8 us_cmd_table0[5] = { 231,226,221,217,212};//12345
 const U8 us_cmd_table1[5] = { 231,226,221,212,217};//12354
 const U8 us_cmd_table2[5] = { 231,226,217,221,212};//12435
 const U8 us_cmd_table3[5] = { 231,226,217,212,221};//12453
 const U8 us_cmd_table4[5] = { 231,226,212,221,217};//12534
 const U8 us_cmd_table5[5] = { 231,226,212,217,221};//12543

 const U8 us_cmd_table6[5] = { 231,221,226,217,212};//13245
 const U8 us_cmd_table7[5] = { 231,221,226,212,217};//13254
 const U8 us_cmd_table8[5] = { 231,221,217,226,212};//13425
 const U8 us_cmd_table9[5] = { 231,221,217,212,226};//13452
 const U8 us_cmd_table10[5] = { 231,221,212,226,217};//13524
 const U8 us_cmd_table11[5] = { 231,221,212,217,226};//13542

 const U8 us_cmd_table12[5] = { 231,217,226,221,212};//14235
 const U8 us_cmd_table13[5] = { 231,217,226,212,221};//14253
 const U8 us_cmd_table14[5] = { 231,217,221,226,212};//14325
 const U8 us_cmd_table15[5] = { 231,217,221,212,226};//14352
 const U8 us_cmd_table16[5] = { 231,217,212,226,221};//14523
 const U8 us_cmd_table17[5] = { 231,217,212,221,226};//14532
 const U8 us_cmd_table18[5] = { 231,212,226,221,217};//15234
 const U8 us_cmd_table19[5] = { 231,212,226,217,221};//15243
 const U8 us_cmd_table20[5] = { 231,212,221,226,217};//15324
 const U8 us_cmd_table21[5] = { 231,212,221,217,226};//15342
 const U8 us_cmd_table22[5] = { 231,212,217,226,221};//15423
 const U8 us_cmd_table23[5] = { 231,212,217,221,226};//15432


///US_SendCMD
U8 recive_send_flag=0;//0 RECIVE  1 SEND
U8 recive_start=0;
U8 tone_init=0;
U8 switch_cmd_cnt=0;
U8 cmd_table;




//US_ReceiveCMD

U16 *freq_cmd;
U16 sample_rate=48000;  //this example uses 16k sample rate

//US_TimerSwitch

U8 tone_table_count=0;
U8 tone_table_count_1=0;
U8 tone_trans_to_target=0;
U8 tone_table_done_flag=0;

//US_DacSendTone
#define tone_peak 20000
U8 fade_in_flag=0;
S32 tone_table_done_temp=0;
S32 temp_root=0;

U8 sin_flag =0;
U32 freq_change_count =0;
extern U16 divisor_tone;
extern U8 divisor_stsDAC;
U16 target_tone;

U32 total_duration_count=0;


U32 freq_duration_ms_time_temp=0;
#define freq_duration_ms_time 40 //ms

const U8 *tone_table_ptr;

//////////////////////////////////////////////////////////////////////
//  Defines  //usapi
//////////////////////////////////////////////////////////////////////
#if _DAC_CH0_TIMER==USE_TIMER1
#define DAC_CH0_TIMER               P_TIMER1
#define DAC_CH0_TRIGER              DAC_TIMER1_TRIG
#elif _DAC_CH0_TIMER==USE_TIMER2
#define DAC_CH0_TIMER               P_TIMER2
#define DAC_CH0_TRIGER              DAC_TIMER2_TRIG
#else
#define DAC_CH0_TIMER               P_TIMER0
#define DAC_CH0_TRIGER              DAC_TIMER0_TRIG
#endif

#if _DAC_CH1_TIMER==USE_TIMER0
#define DAC_CH1_TIMER               P_TIMER0
#define DAC_CH1_TRIGER              DAC_TIMER0_TRIG
#elif _DAC_CH1_TIMER==USE_TIMER2
#define DAC_CH1_TIMER               P_TIMER2
#define DAC_CH1_TRIGER              DAC_TIMER2_TRIG
#else
#define DAC_CH1_TIMER               P_TIMER1
#define DAC_CH1_TRIGER              DAC_TIMER1_TRIG
#endif

//////////FOR Ultrasound founction usapi End    ***************



//usapi
U16 WM_Flag =0;
U16 WM_Flag_temp[6] ={9};
U16 WM_Flag_temp_ub[6] ={9};

//static S32 OutputBuf[FFT_HSIZE];
//static f;
//#include "Butterfly_FFT.h"

/* Defines -------------------------------------------------------------------*/
#if 1  //CANT DELETE IT Ed
#define AUDIO_OUT_SIZE     128//256//256//320
#define AUDIO_OUT_COUNT    3
#define AUDIO_BUF_SIZE     (AUDIO_OUT_SIZE*AUDIO_OUT_COUNT)//Ed_test buf
//#define DAC_BUF_SIZE       SPEED_CTL_OUT_SIZE*14

#endif



#define AGC_ENALBE          0

#define TARGET_RMS     	MINUS_7DB                           // Adjustable, Target RMS signal magnitude 			
#define ALPHA           4096//4096//4096                    // 0.125; coeff. of 1st order IIR LPF 
#define BETA            1024//(2048/2)                      // 0.125; step size constant 

//echo cancellor by S/W volume control
#define ECHO_CANCELL    0
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define EC_ALPHA					4096
#define EC_ALPHA_1					32767 - ALPHA
#define EC_BETA 					2048
#define EC_VOLUME_MIN				8192
#define EC_VOLUME_MAX				32767 - EC_VOLUME_MIN

#if 0
static S32 EC_Env = 0;
static U32 EC_envelope = 0;
static U16 EC_FrameIndex = 0;
static S16 EC_volume = EC_VOLUME_MAX;
#endif

#define NOISE_GATE_ENABLE   0//0

#define NG_ON_OFF_SAMPLE_16K        160//320//160//10ms
#define NG_ATTACK_SAMPLE_16K        80//160//80//5ms
#define NG_RELEASE_SAMPLE_16K       (320*5)//100ms

#define NG_ON_OFF_SAMPLE_8K         80//10ms
#define NG_ATTACK_SAMPLE_8K         40//5ms
#define NG_RELEASE_SAMPLE_8K        (160*5)//100ms

#define SMOOTH_ENABLE          1

#define HPF_ENABLE          0//0

#define HPF_ORDER		    2
#define HPF_QFORMAT         14
/* Static Functions ----------------------------------------------------------*/

/* Constant Table ------------------------------------------------------------*/
//static const U16 VolTbl[16]={
//    0,
//    2920-DAC_DATA_OFFSET,
//    3471-DAC_DATA_OFFSET,
//    4125-DAC_DATA_OFFSET,
//    4903-DAC_DATA_OFFSET,
//    5827-DAC_DATA_OFFSET,
//    6925-DAC_DATA_OFFSET,
//    8231-DAC_DATA_OFFSET,
//    9783-DAC_DATA_OFFSET,
//    11627-DAC_DATA_OFFSET,
//    13818-DAC_DATA_OFFSET,
//    16423-DAC_DATA_OFFSET,
//    19519-DAC_DATA_OFFSET,
//    23198-DAC_DATA_OFFSET,
//    27571-DAC_DATA_OFFSET,
//    32767-DAC_DATA_OFFSET,
//}; 

#if 0//_RT_PITCH_CHANGE
static const float PitchRatioTbl[25]={
    0.5,
    0.53,
    0.56,
    0.59,
    0.63,
    0.67,
    0.72,
    0.75,
    0.79,
    0.84,
    0.89,
    0.94,
    1,
    1.06,
    1.12,
    1.19,
    1.26,
    1.33,
    1.41,
    1.50,
    1.59,
    1.68,
    1.78,
    1.89,
    2,
};  
#endif



#if 0
//Q14 ,cutoff 200Hz
static const S16 CoeffB_16K[HPF_ORDER+1] = {
    15522,
    -31044,
    15522
};
 
static const S16 CoeffA_16K[HPF_ORDER+1] = {
    16384,
    -30949,
    14661
};


static const S16 CoeffB_8K[HPF_ORDER+1] = {
    14660,
    -29320,
    14660
};
 
static const S16 CoeffA_8K[HPF_ORDER+1] = {
    16384,
    -29141,
    13120
};
#endif
/* Static Variables ----------------------------------------------------------*/
static volatile struct AUDIO_BUF_CTL{
    S32 process_count;	//decode/encode count
    S32 fifo_count;	    //decode/encode count
    U32 samples;	    //samples
#if 0//_RT_PITCH_CHANGE
    U32 sample_count;	    //samples
    U32 fifo_count2;	    //decode/encode count
#endif       
    //U16 vol;            //volume
    U16 sample_rate;    //sample_rate
    U16 out_size;        //audio frame size
    U16 in_size;         //data frame size
    U8 state;           //channel state
    U8 shadow_state;    //channel shadow state
//    U8 ramp;            //smooth ramp
    U8 out_count;
    U8 index;
}AudioBufCtl;
//
//#if _PCM_RECORD
//static U32 StorageIndex;
//#endif

#if AGC_ENALBE
static U8 AgcCnt;
#endif

#if 0
static S32 y[HPF_ORDER+1]; //output samples
static S16 x[HPF_ORDER+1]; //input samples
static S16 const*CoeffB,*CoeffA;

static U32 RecordSize;
#endif

static U16 AudioVol;
//static U8  OpMode;
static U8  ChVol;

typedef struct{
    //U16 *buf;
    S16 *buf;
    U16 size;
    U16 offset;
}BUF_CTL_T;   

static volatile  BUF_CTL_T BufCtl, DacBufCtl;
//static volatile U16 DacBufWrOffset;
//static volatile U32 DacBufWrCnt;

#if SMOOTH_ENABLE
static volatile struct{
//    U32 p_index;            //play index
    S16 count;              //sample count in process
    U8  step_sample;        //sample size for each step
    U8  smooth;             //up or down or none
    U8  state;              //current state
//    U8  p_parameter;        //play paramete
}SmoothCtl;
#endif

static const U8 Signature[]="NX1";

#if 0//_RT_PITCH_CHANGE
static struct{
    U16 index;
    U8 speed_ratio_index;
    U8 pitch_ratio_index;
}SpeedCtl;
#endif








#if 0
static U8  StorageMode;
static U32 StartAddr,CurAddr;
#endif

/* Dynamic Allocate RAM-------------------------------------------------------*/
#if _USE_HEAP_RAM
static  S16 *AudioBuf=NULL;
//static  S16 *DacBuf=NULL;
#if 0//_RT_PITCH_CHANGE
static U16 *SpeedCtlBuf=NULL;
#endif



#if AGC_ENALBE
static U16 *AgcBuf=NULL;
#endif

/* Fixed RAM  ---------------------------------------------------------------*/
#else
//static  U16 AudioBuf[AUDIO_BUF_SIZE];//Ed_test buf
static  S16 AudioBuf[AUDIO_BUF_SIZE];//Ed_test buf
//static  S16 DacBuf[DAC_BUF_SIZE];
#if 0//_RT_PITCH_CHANGE
static  U16 SpeedCtlBuf[AUDIO_OUT_SIZE+SPEED_CTL_FRAME_SIZE];
#endif





#if AGC_ENALBE
static U16 AgcBuf[AGC_FRAME_SIZE];
#endif


#endif

/* Extern Functions ----------------------------------------------------------*/
//extern void CB_US_PlayStart(void);
//extern void CB_US_InitDac(CH_TypeDef *chx,U16 sample_rate);
extern void CB_US_DacCmd(CH_TypeDef *chx,U8 cmd);
//extern void CB_US_ChangeSR(CH_TypeDef *chx,U16 sample_rate);
extern void CB_US_InitAdc(U16 sample_rate);
extern void CB_US_AdcCmd(U8 cmd);
//extern void CB_US_WriteDac(U8 size,U16* psrc16);
//------------------------------------------------------------------//
// Allocate PCM memory
// Parameter: 
//          func: Choose Allocate function
// return value:
//          NONE
//------------------------------------------------------------------// 
#if _USE_HEAP_RAM
static void US_AllocateMemory(U16 func)
{
    if(AudioBuf==NULL && (func&MEMORY_ALLOCATE_PLAY))    // AudioBuf & SpeedCtlBuf together
    {
        AudioBuf = (S16 *)MemAlloc(AUDIO_BUF_SIZE<<1);
#if AGC_ENALBE
        AgcBuf = (U16 *)MemAlloc(AGC_FRAME_SIZE<<1);
#endif        
#if 0//_RT_PITCH_CHANGE
        SpeedCtlBuf = (U16 *)MemAlloc((AUDIO_OUT_SIZE+SPEED_CTL_FRAME_SIZE)<<1);
#endif
        //DacBuf = (S16 *)MemAlloc(DAC_BUF_SIZE<<1);
       
    }
   
}
//------------------------------------------------------------------//
// Free RT memory
// Parameter: 
//          func: Choose Allocate function
// return value:
//          NONE
//------------------------------------------------------------------// 
static void US_FreeMemory(U16 func)
{
    if(AudioBuf!=NULL && (func&MEMORY_ALLOCATE_PLAY))    
    {
        MemFree(AudioBuf);
        AudioBuf=NULL;
#if AGC_ENALBE
        MemFree(AgcBuf);
        AgcBuf=NULL;
#endif        
#if 0//_RT_PITCH_CHANGE
        MemFree(SpeedCtlBuf);
        SpeedCtlBuf=NULL;
#endif
        //MemFree(DacBuf);
        //DacBuf = NULL;
    }
    if(func&MEMORY_ALLOCATE_RECORD)  
    {

    }
}
#endif

//------------------------------------------------------------------//
// Get current status
// Parameter: 
//          NONE
// return value:
//          status:STATUS_PLAYING,STATUS_PAUSE,STATUS_STOP
//------------------------------------------------------------------// 
U8 US_GetStatus(void){
    return AudioBufCtl.state;    
}

//------------------------------------------------------------------//
// Set audio volume
// Parameter: 
//          vol:0~15
// return value:
//          NONE
//------------------------------------------------------------------// 
void US_SetVolume(U8 vol){   
    if(vol>CH_VOL_15){
        vol=CH_VOL_15;
    }    
    ChVol=vol;  
    AudioVol = ChVol <<11;
}

//------------------------------------------------------------------//
// Codec init
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------// 
void US_Init(void){

    ChVol=CH_VOL_15;    
    

}
//------------------------------------------------------------------//
// Stop record voice
// Parameter:
//          index: file index or absolute address
//          mode:SPI_MODE,SD_MODE
//          sample_rate:sample rate
//          size: record size
// return value:
//          0:ok
//          others:fail
//------------------------------------------------------------------//
void US_Stop(void)
{
	
    CB_US_AdcCmd(DISABLE);
#if _US_PLAY_AUDIO_CH==0
    CB_US_DacCmd(DAC_CH0,DISABLE);
#else
    CB_US_DacCmd(DAC_CH1,DISABLE);
#endif

#if _USE_HEAP_RAM							//20171228
    US_FreeMemory(MEMORY_ALLOCATE_PLAY);	//20171228
#endif										//20171228

	AudioBufCtl.state=STATUS_STOP;
}
//------------------------------------------------------------------//
// Start record voice
// Parameter: 
//          index: file index or absolute address
//          mode:SPI_MODE,SD_MODE
//          sample_rate:sample rate
//          size: record size
// return value:
//          0:ok
//          others:fail
//------------------------------------------------------------------// 
U8 US_Play(U16 sample_rate)
{
    //U8 i;

//#if _USE_HEAP_RAM								//20171228    
//    RT_AllocateMemory(MEMORY_ALLOCATE_PLAY);	//20171228	
//#endif    									//20171228
    memset((void *)&AudioBufCtl, 0, sizeof(AudioBufCtl));

    AudioBufCtl.sample_rate=sample_rate;
    AudioBufCtl.out_size=AUDIO_OUT_SIZE;
    AudioBufCtl.in_size=(AUDIO_OUT_SIZE<<1);

    //dprintf("AudioBufCtl out %d in %d\r\n",AudioBufCtl.out_size,AudioBufCtl.in_size);

  
//    StorageIndex=index;

#if AGC_ENALBE    
    AgcCnt=0;
    AgcInit(ALPHA,BETA,TARGET_RMS);   
#endif
     
       
    AudioBufCtl.out_count=AUDIO_BUF_SIZE/AudioBufCtl.out_size;
    BufCtl.offset=0;   
    BufCtl.size=AudioBufCtl.out_size*AudioBufCtl.out_count;
    BufCtl.buf=AudioBuf;
    
    
       
    CB_US_InitAdc(sample_rate);   
    //RecordSize=size;
    
//----------------Play ---------------------------------
/*
#if _RT_PLAY_AUDIO_CH==0
    CB_US_InitDac(DAC_CH0,sample_rate);
#else
    CB_US_InitDac(DAC_CH1,sample_rate);
#endif
*/
#if 0//_RT_PITCH_CHANGE
    SpeedCtl.pitch_ratio_index=RATIO_INIT_INDEX;
    SpeedCtl.index=0;
//    TimeStretchInit(0,1/PitchRatioTbl[SpeedCtl.pitch_ratio_index]);
#if _US_PLAY_AUDIO_CH==0    
    CB_US_ChangeSR(DAC_CH0,AudioBufCtl.sample_rate*PitchRatioTbl[SpeedCtl.pitch_ratio_index]);
#else
    CB_US_ChangeSR(DAC_CH1,AudioBufCtl.sample_rate*PitchRatioTbl[SpeedCtl.pitch_ratio_index]);
#endif
#endif         
    memset((void *)AudioBuf, 0, AUDIO_BUF_SIZE*2);

    //DacBufCtl.offset = 0;
    //DacBufCtl.size = DAC_BUF_SIZE;
    //DacBufCtl.buf = DacBuf;
    //DacBufWrOffset = 0;
   // DacBufWrCnt = 0;
    
    //dprintf("DacBufCtl.size %d\r\n",DacBufCtl.size);

    //DecodeFrame();
    
    //OpMode=0;
    AudioBufCtl.state=  STATUS_PLAYING;
    AudioBufCtl.shadow_state=AudioBufCtl.state;      
    
    CB_US_AdcCmd(ENABLE);
    //CB_US_PlayStart();

    return 0;
}
//extern const U8  VOICEFILEX[ ];
//U32 VoiceIdx=0;
//------------------------------------------------------------------//
// Adc ISR
// Parameter: 
//          size: data-buffer fill size 
//       pdest32: data-buffer address
// return value:
//          NONE
//------------------------------------------------------------------// 
void US_AdcIsr(U8 size, U32 *pdest32){
    S16 *pdst16;
    U8 i;
    //U32 temp;
    S16 temp;//Ed_test
    if(AudioBufCtl.state==STATUS_PLAYING){
        //GPIOC->Data ^=0x01;
      	
        temp=BufCtl.size-BufCtl.offset;
        if(size>temp){
            size=temp;
        }   

#if AGC_ENALBE        
        temp=AGC_FRAME_SIZE-AgcCnt;
        if(size>temp){
            size=temp;
        }   
#endif        
        /*
        if((AudioBufCtl.fifo_count+size-AudioBufCtl.process_count)>(AudioBufCtl.out_count*AudioBufCtl.out_size)){
            for(i=0;i<size;i++){
                temp=(*pdest32-32768);    
            }   
            //dprintf("E fifo %d p_count %d\r\n",AudioBufCtl.fifo_count,AudioBufCtl.process_count);
            size=0;
        }
        */
        //dprintf("size,%d\r\n",size);
        AudioBufCtl.fifo_count+=size;   

        

        pdst16=(S16 *)(BufCtl.buf+BufCtl.offset);
        
        for(i=0;i<size;i++){
            temp=((*pdest32++)-32768);   
//dprintf("temp %d   %d \r\n",temp,*pdest32);
            *pdst16++= temp;
#if AGC_ENALBE            
            AgcBuf[AgcCnt+i]=temp;
#endif            
//            temp=(*pdest32-32768);     
//            *pdst16++= (VOICEFILEX[VoiceIdx]|(VOICEFILEX[VoiceIdx+1]<<8));
//            VoiceIdx+=2;
        }   
        BufCtl.offset+=size;

#if AGC_ENALBE        
        AgcCnt+=size;
        if(AgcCnt==AGC_FRAME_SIZE){
            AgcCnt=0;

            AgcProcess((S16 *)AgcBuf,AGC_FRAME_SIZE);   
        }     
#endif           
        if(BufCtl.offset==BufCtl.size){
            BufCtl.offset=0;    
        }       
    }             
}   







//------------------------------------------------------------------//
// Encode one frame
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//  
//static void EncodeFrame(void)
static U16* EncodeFrame(void)
{
    //U16 *audio_buf;
    S16 *audio_buf;//Ed_test
    //U32 speed_ctl_size,sum=0;
    //S16 *dac_buf;
    //U8 n,Vad_flag;
    //U16 i;
    //S32 out;
    //S16 WM_DataArray[AUDIO_OUT_SIZE];//Ed_test
    //S16 WM_DataArray[128];//Ed_test
    
    static U8 temp_cnt=1;
	
    audio_buf=AudioBuf+AudioBufCtl.out_size*AudioBufCtl.index;

//dprintf("EncodeFrame \r\n");




WM_Flag=FFT_TransForm_128(audio_buf);//Ed_test clare
//dprintf("%d  ",WM_Flag);

if(WM_Flag_temp[temp_cnt-1] != WM_Flag && WM_Flag!=0)
{

	if(WM_Flag_temp[1]==1)
	{
			WM_Flag_temp[temp_cnt]=WM_Flag;
			
			WM_Flag_temp_ub[temp_cnt]=WM_Flag_temp[temp_cnt];
			
			temp_cnt++;
			//dprintf("\r\n***%d   WM[1]=%d WM[2]=%d WM [3]=%d WM[4]=%d WM[5]=%d\r\n",WM_Flag,WM_Flag_temp[1],WM_Flag_temp[2],WM_Flag_temp[3],WM_Flag_temp[4],WM_Flag_temp[5]);
		
		
			AudioBufCtl.process_count+=AudioBufCtl.out_size;//Ed_test 
		
			
		
			if(temp_cnt==6)//cnt
			{
				temp_cnt=1;
				WM_Flag_temp[0]=9;//init
				WM_Flag_temp[1]=0;
				WM_Flag_temp[2]=0;
				WM_Flag_temp[3]=0;
				WM_Flag_temp[4]=0;
				WM_Flag_temp[5]=0;
				
				return WM_Flag_temp_ub;
			}
			else
			{
		
				return 0;
			}
	}
	else
	{
	//dprintf("\r\n---%d--  WM[1]=%d WM[2]=%d WM[3]=%d\r\n",WM_Flag,WM_Flag_temp[1],WM_Flag_temp[2],WM_Flag_temp[3]);
		WM_Flag_temp[1]=WM_Flag;
		temp_cnt=1;
	}

}

//WaterMark_Detect 






    AudioBufCtl.index++;
    if(AudioBufCtl.index==AudioBufCtl.out_count){
        AudioBufCtl.index=0;   
    }    




AudioBufCtl.process_count+=AudioBufCtl.out_size;//Ed_test 

return 0;




    






}
//------------------------------------------------------------------//
// Service loop
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//     
//void RT_ServiceLoop(void)
U16* US_ServiceLoop(void)
{
    if(AudioBufCtl.state==STATUS_PLAYING)
    {
        if((AudioBufCtl.fifo_count-AudioBufCtl.process_count)>=AudioBufCtl.out_size)
        {            
            //EncodeFrame();            			
            return EncodeFrame();
        }          
    }   
    return 0;
} 












































////////FOR Ultrasound founction  usapi start*******************************************************************************
//				  17250 17625 18000 18375 18750 
//                 1     2       3      4      5     
//				138800 141600  145200 148000 151200 
//tone_conunt     230    225    219    216    211
//updample        57     56     54     54     52


//				  17250 17625 18000 18375 18750 
//U16 table2[6]={ 34700,35400,36300,37000,37800,38500,39200,39800};  //42000 =>mean silence
//                  1     2     3     4     5    6     7     8

U8 AUDIO_GetStatus_US(void){
    U8 sts;    
    sts=STATUS_STOP;



#if _ADPCM_PLAY    
        sts|=ADPCM_GetStatus();
#endif  
#if _ADPCM_CH2_PLAY    
        sts|=ADPCM_CH2_GetStatus();
#endif  
#if _ADPCM_CH3_PLAY    
        sts|=ADPCM_CH3_GetStatus();
#endif  
#if _ADPCM_CH4_PLAY    
        sts|=ADPCM_CH4_GetStatus();
#endif  
#if _SBC_PLAY    
        sts|=SBC_GetStatus();
#endif   
#if _SBC_CH2_PLAY    
        sts|=SBC_CH2_GetStatus();
#endif   
#if _CELP_PLAY    
        sts|=CELP_GetStatus();
#endif   
#if _MIDI_PLAY    
        sts|=MIDI_GetStatus();
#endif 
#if _PCM_PLAY    
        sts|=PCM_GetStatus();
#endif 
#if _PCM_CH2_PLAY    
        sts|=PCM_CH2_GetStatus();
#endif 
#if _PCM_CH3_PLAY    
        sts|=PCM_CH3_GetStatus();
#endif 






    return sts;
}

void cmd_arry_init(void)
{
	freq_cmd[0] =9;
	freq_cmd[1] =0;
	freq_cmd[2] =0;
	freq_cmd[3] =0;
	freq_cmd[4] =0;
	freq_cmd[5] =0;
}



void US_SendCMD(U8 cmd)
{


	US_Stop();
	cmd_table=cmd;//shitch cmd table
	switch_cmd_cnt=0;//init cmd shift number
	recive_send_flag=1;//switch send
	recive_start=0;//switch recive
					 					
   	tone_init=0;






	if(recive_send_flag==1)//FOR SEND  recive_send_flag==1
	{
	
	
	
	
		switch (cmd)
		{
		
			case 0:
				tone_table_ptr=us_cmd_table0;break;
			case 1:
				tone_table_ptr=us_cmd_table1;break;
			case 2:
				tone_table_ptr=us_cmd_table2;break;	
			case 3:
				tone_table_ptr=us_cmd_table3;break;
			case 4:
				tone_table_ptr=us_cmd_table4;break;
			case 5:
				tone_table_ptr=us_cmd_table5;break;
			case 6:
				tone_table_ptr=us_cmd_table6;break;
			case 7:
				tone_table_ptr=us_cmd_table7;break;
			case 8:
				tone_table_ptr=us_cmd_table8;break;
			case 9:
				tone_table_ptr=us_cmd_table9;break;			
			case 10:
				tone_table_ptr=us_cmd_table10;break;
			case 11:
				tone_table_ptr=us_cmd_table11;break;
			case 12:
				tone_table_ptr=us_cmd_table12;break;
			case 13:
				tone_table_ptr=us_cmd_table13;break;
			case 14:
				tone_table_ptr=us_cmd_table14;break;
			case 15:
				tone_table_ptr=us_cmd_table15;break;
			case 16:
				tone_table_ptr=us_cmd_table16;break;
			case 17:
				tone_table_ptr=us_cmd_table17;break;
			case 18:
				tone_table_ptr=us_cmd_table18;break;
			case 19:
				tone_table_ptr=us_cmd_table19;break;
			case 20:
				tone_table_ptr=us_cmd_table20;break;
			case 21:
				tone_table_ptr=us_cmd_table21;break;
			case 22:
				tone_table_ptr=us_cmd_table22;break;	
			case 23:
				tone_table_ptr=us_cmd_table23;break;		
			
							
				
				
			default:
				break;
		
		
		
		}//switch
	
	
	
	
	
		if(tone_init==0)
		{

	          	
#if _US_PLAY_AUDIO_CH==HW_CH0
				divisor_stsDAC=1;
	          	CB_US_InitDac(DAC_CH0,138800);
	          	CB_US_DacCmd(DAC_CH0,ENABLE);
#else
				divisor_stsDAC=2;
				CB_US_InitDac(DAC_CH1,138800);
				CB_US_DacCmd(DAC_CH1,ENABLE);
#endif	          	
				divisor_stsDAC=0;
			
			tone_init=1;
		}
	
	
	
	}//FOR SEND  recive_send_flag==1

}


S8 US_ReceiveCMD(void)
{

	//S8 return_cmd=-1;
	

	
	if(recive_send_flag==0 && AUDIO_GetStatus_US()==0)//GetStatus=1 =>playing  //for receive
	{
	             
		#if _USE_HEAP_RAM								//20171228
			AUDIO_ServiceLoop();
			US_AllocateMemory(MEMORY_ALLOCATE_PLAY);	//20171228	
		#endif    										//20171228
		
		if(recive_start== 0 )
		{
			
		  	US_Play(sample_rate);//INIT REALTIME ADC 
		  	recive_start=1;
		
		}
	
	
		freq_cmd = US_ServiceLoop();


		
	////////////////////////////////////////////////////////////////////////////////////////
		if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==3 && freq_cmd[4] ==4 && freq_cmd[5] ==5)//table0
		{	cmd_arry_init();	return 0;	}
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==3 && freq_cmd[4] ==5 && freq_cmd[5] ==4)//table1
		{	cmd_arry_init();	return 1;	}
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==4 && freq_cmd[4] ==3 && freq_cmd[5] ==5)//table2
		{	cmd_arry_init();	return 2;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==4 && freq_cmd[4] ==5 && freq_cmd[5] ==3)//table3
		{	cmd_arry_init();	return 3;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==5 && freq_cmd[4] ==3 && freq_cmd[5] ==4)//table4
		{	cmd_arry_init();	return 4;	}			
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==2 && freq_cmd[3] ==5 && freq_cmd[4] ==4 && freq_cmd[5] ==3)//table5
		{	cmd_arry_init();	return 5;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==2 && freq_cmd[4] ==4 && freq_cmd[5] ==5)//table6
		{	cmd_arry_init();	return 6;	}			
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==2 && freq_cmd[4] ==5 && freq_cmd[5] ==4)//table7
		{	cmd_arry_init();	return 7;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==4 && freq_cmd[4] ==2 && freq_cmd[5] ==5)//table8
		{	cmd_arry_init();	return 8;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==4 && freq_cmd[4] ==5 && freq_cmd[5] ==2)//table9
		{	cmd_arry_init();	return 9;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==5 && freq_cmd[4] ==2 && freq_cmd[5] ==4)//table10
		{	cmd_arry_init();	return 10;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==3 && freq_cmd[3] ==5 && freq_cmd[4] ==4 && freq_cmd[5] ==2)//table11
		{	cmd_arry_init();	return 11;	}
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==2 && freq_cmd[4] ==3 && freq_cmd[5] ==5)//table12
		{	cmd_arry_init();	return 12;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==2 && freq_cmd[4] ==5 && freq_cmd[5] ==3)//table13
		{	cmd_arry_init();	return 13;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==3 && freq_cmd[4] ==2 && freq_cmd[5] ==5)//table14
		{	cmd_arry_init();	return 14;	}			
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==3 && freq_cmd[4] ==5 && freq_cmd[5] ==2)//table15
		{	cmd_arry_init();	return 15;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==5 && freq_cmd[4] ==2 && freq_cmd[5] ==3)//table16
		{	cmd_arry_init();	return 16;	}			
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==4 && freq_cmd[3] ==5 && freq_cmd[4] ==3 && freq_cmd[5] ==2)//table17
		{	cmd_arry_init();	return 17;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==2 && freq_cmd[4] ==3 && freq_cmd[5] ==4)//table18
		{	cmd_arry_init();	return 18;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==2 && freq_cmd[4] ==4 && freq_cmd[5] ==3)//table19
		{	cmd_arry_init();	return 19;	}		
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==3 && freq_cmd[4] ==2 && freq_cmd[5] ==4)//table20
		{	cmd_arry_init();	return 20;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==3 && freq_cmd[4] ==4 && freq_cmd[5] ==2)//table21
		{	cmd_arry_init();	return 21;	}
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==4 && freq_cmd[4] ==2 && freq_cmd[5] ==3)//table22
		{	cmd_arry_init();	return 22;	}	
		else if (freq_cmd[1] ==1 && freq_cmd[2] ==5 && freq_cmd[3] ==4 && freq_cmd[4] ==3 && freq_cmd[5] ==2)//table23
		{	cmd_arry_init();	return 23;	}	


	
	}

	return -1;

}




#if _US_PLAY_AUDIO_CH == HW_CH0

void US_DacSendTone_CH0(void)
{


if(recive_send_flag==1)
{

		
		if(fade_in_flag==0 )
		{
			
			
			temp_root=(tone_table_done_temp*0.7);
			if(sin_flag==0)
			{

			P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = temp_root;
	   		P_DAC->CH0.CH_Data = tone_table_done_temp;
	   		P_DAC->CH0.CH_Data = temp_root;

			sin_flag=1;
			}
			else //if(sin_flag==1)
			{

			P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = -temp_root;
	   		P_DAC->CH0.CH_Data = -tone_table_done_temp;
	   		P_DAC->CH0.CH_Data = -temp_root;
	   		
	   		tone_table_done_temp+=100;
			sin_flag=0;
			}

			
			
			if(tone_table_done_temp>=tone_peak)
			{
				tone_table_done_temp=tone_peak;

				
				fade_in_flag=1;
				tone_table_count=1;
				tone_table_count_1=0;
				
				tone_table_done_flag=0;
				freq_duration_ms_time_temp =  SysTick;
				
			}
		
		
		}
		else if(tone_table_done_flag==0 && fade_in_flag==1)
		{
			if(sin_flag==0)
			{
	   		P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = 14142;
	   		P_DAC->CH0.CH_Data = 20000;
	   		P_DAC->CH0.CH_Data = 14142;
	   		sin_flag=1;
	   		}
			else //if(sin_flag==1)
			{
	   		P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = -14142;
	   		P_DAC->CH0.CH_Data = -20000;
	   		P_DAC->CH0.CH_Data = -14142;   		
			sin_flag=0;
			freq_change_count++;
			
			total_duration_count++;
			
			}
			
		}
		else if(tone_table_done_flag==1 && fade_in_flag==1)
		{

			temp_root=(tone_table_done_temp*0.7);
			//temp_root=(tone_table_done_temp/2);
			if(sin_flag==0)
			{


			P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = temp_root;
	   		P_DAC->CH0.CH_Data = tone_table_done_temp;
	   		P_DAC->CH0.CH_Data = temp_root;

			sin_flag=1;
			}
			else //if(sin_flag==1)
			{

			P_DAC->CH0.CH_Data = 0;
	   		P_DAC->CH0.CH_Data = -temp_root;
	   		P_DAC->CH0.CH_Data = -tone_table_done_temp;
	   		P_DAC->CH0.CH_Data = -temp_root;
	   		
	   		
			sin_flag=0;
			
			tone_table_done_temp-=100;
			}

			


			
			if(tone_table_done_temp<=0)
			{

	   			//////////////////init
				tone_table_done_flag=0;
				recive_send_flag=0;
				tone_table_done_temp=0;
				fade_in_flag=0;
				total_duration_count=0;
				

				
				CB_US_DacCmd(DAC_CH0,DISABLE);
			}


		}
		







		
		if(freq_change_count >= 8 && tone_trans_to_target==1)/////freq_change_count  switch  freq17250 5000=>292ms  17=>1ms
		{
		
		/*
			if( (divisor_tone-1)<target_tone || (divisor_tone+1)>target_tone)
			{
				divisor_tone=target_tone;
				DAC_CH0_TIMER->Data = divisor_tone;	
				tone_trans_to_target=0;
			}
			else */
			if(divisor_tone>target_tone)
			{
				divisor_tone-=1;
				DAC_CH0_TIMER->Data = divisor_tone;	
			}
			else if(divisor_tone<target_tone)
			{
				divisor_tone+=1;
				DAC_CH0_TIMER->Data = divisor_tone;	
			}
			else if(divisor_tone == target_tone)
			{
				tone_trans_to_target=0;
			}
		
			freq_change_count=0;//init  freq_change_count:how offen to change.  unit:sin tone cycle
			
		}
		
		
		
		
		
		
		
		if( ((SysTick-freq_duration_ms_time_temp)>=freq_duration_ms_time) && tone_table_done_flag==0 )//one freq duration 40ms
		{
			static U16 i=0;
			freq_duration_ms_time_temp = SysTick;
			
			if(tone_table_count<5 && tone_table_count_1<5)  
			{
			
				target_tone=tone_table_ptr[tone_table_count];
				tone_trans_to_target=1;
				tone_table_count++;
				
				//
				if(tone_table_count==5) 
				{
				tone_table_count=0;
				tone_table_count_1++;
				}
				
				
			}
			else if(tone_table_count_1>=5) 
			{
				i++;
				if(i>=2)
				{
					tone_table_done_flag=1;
				}
			}
		
		
			
		}
		
		
		
}//	recive_send_flag==1	


}


#else


void US_DacSendTone_CH1(void)
{


if(recive_send_flag==1)
{

		
		if(fade_in_flag==0 )
		{
			
			
			temp_root=(tone_table_done_temp*0.7);
			if(sin_flag==0)
			{

			P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = temp_root;
	   		P_DAC->CH1.CH_Data = tone_table_done_temp;
	   		P_DAC->CH1.CH_Data = temp_root;

			sin_flag=1;
			}
			else //if(sin_flag==1)
			{

			P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = -temp_root;
	   		P_DAC->CH1.CH_Data = -tone_table_done_temp;
	   		P_DAC->CH1.CH_Data = -temp_root;
	   		
	   		tone_table_done_temp+=100;
			sin_flag=0;
			}

			
			
			if(tone_table_done_temp>=tone_peak)
			{
				tone_table_done_temp=tone_peak;

				
				fade_in_flag=1;
				tone_table_count=1;
				tone_table_count_1=0;
				
				tone_table_done_flag=0;
				freq_duration_ms_time_temp =  SysTick;
				
			}
		
		
		}
		else if(tone_table_done_flag==0 && fade_in_flag==1)
		{
			if(sin_flag==0)
			{
	   		P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = 14142;
	   		P_DAC->CH1.CH_Data = 20000;
	   		P_DAC->CH1.CH_Data = 14142;
	   		sin_flag=1;
	   		}
			else //if(sin_flag==1)
			{
	   		P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = -14142;
	   		P_DAC->CH1.CH_Data = -20000;
	   		P_DAC->CH1.CH_Data = -14142;   		
			sin_flag=0;
			freq_change_count++;
			
			total_duration_count++;
			
			}
			
		}
		else if(tone_table_done_flag==1 && fade_in_flag==1)
		{

			temp_root=(tone_table_done_temp*0.7);
			//temp_root=(tone_table_done_temp/2);
			if(sin_flag==0)
			{


			P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = temp_root;
	   		P_DAC->CH1.CH_Data = tone_table_done_temp;
	   		P_DAC->CH1.CH_Data = temp_root;

			sin_flag=1;
			}
			else //if(sin_flag==1)
			{

			P_DAC->CH1.CH_Data = 0;
	   		P_DAC->CH1.CH_Data = -temp_root;
	   		P_DAC->CH1.CH_Data = -tone_table_done_temp;
	   		P_DAC->CH1.CH_Data = -temp_root;
	   		
	   		
			sin_flag=0;
			
			tone_table_done_temp-=100;
			}

			


			
			if(tone_table_done_temp<=0)
			{

	   			//////////////////init
				tone_table_done_flag=0;
				recive_send_flag=0;
				tone_table_done_temp=0;
				fade_in_flag=0;
				total_duration_count=0;
				

				
				CB_US_DacCmd(DAC_CH1,DISABLE);
			}


		}
		







		
		if(freq_change_count >= 8 && tone_trans_to_target==1)/////freq_change_count  switch  freq17250 5000=>292ms  17=>1ms
		{
		
		/*
			if( (divisor_tone-1)<target_tone || (divisor_tone+1)>target_tone)
			{
				divisor_tone=target_tone;
				DAC_CH0_TIMER->Data = divisor_tone;	
				tone_trans_to_target=0;
			}
			else */
			if(divisor_tone>target_tone)
			{
				divisor_tone-=1;
				DAC_CH1_TIMER->Data = divisor_tone;	
			}
			else if(divisor_tone<target_tone)
			{
				divisor_tone+=1;
				DAC_CH1_TIMER->Data = divisor_tone;	
			}
			else if(divisor_tone == target_tone)
			{
				tone_trans_to_target=0;
			}
		
			freq_change_count=0;//init  freq_change_count:how offen to change.  unit:sin tone cycle
			
		}
		
		
		
		
		
		
		
		if( ((SysTick-freq_duration_ms_time_temp)>=freq_duration_ms_time) && tone_table_done_flag==0 )//one freq duration 40ms
		{
			static U16 i=0;
			freq_duration_ms_time_temp = SysTick;
			
			if(tone_table_count<5 && tone_table_count_1<5)  
			{
			
				target_tone=tone_table_ptr[tone_table_count];
				tone_trans_to_target=1;
				tone_table_count++;
				
				//
				if(tone_table_count==5) 
				{
				tone_table_count=0;
				tone_table_count_1++;
				}
				
				
			}
			else if(tone_table_count_1>=5) 
			{
				i++;
				if(i>=2)
				{
					tone_table_done_flag=1;
				}
			}
		
		
			
		}
		
		
		
}//	recive_send_flag==1	


}
#endif

//////////FOR Ultrasound founction usapi End





















   
#endif  //#if _US_PLAY  ALL FUNCTION
