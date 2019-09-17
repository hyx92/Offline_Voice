#include "include.h"
#include "nx1_adc.h"
#include "nx1_gpio.h"
#include "nx1_spi.h"
#include "nx1_wdt.h"
#include "VRCmdTab.h"
#include <string.h>
#include "utility_define.h"
#include "debug.h"

#if _VR_FUNC 

#define BUF0_READY          0
#define BUF1_READY          1
#define BUF_NOTREADY        2

#define MAX_TIME			300

#define BUF0_OFFSET        	0    
#define BUF1_OFFSET         (BUF0_OFFSET+(BUF_SIZE*2))
#define ENGINE_BUF_OFFSET   (BUF1_OFFSET+(BUF_SIZE*2))

#define CSPOTTER_SUCCESS				(0)
#define CSPOTTER_ERR_SDKError			(-2000)
#define CSPOTTER_ERR_NeedMoreSample		(CSPOTTER_ERR_SDKError - 9)

#define NUM_UTTR			(3+1)
#define UTTR_BUF_SIZE		0x1000

static S16 *Buf0,*Buf1;
static U8 BufToggle;
static U16 BufIdx;
static U8 *CyBaseAddr, **ModelAddr;
static U8 numGroup;
static U8 firstTimeFlag = 0;
static U8 VRStateRam[VR_STATE_SIZE];
static volatile U8 BufReady;

U32 hCSpotter;
#if _VR_VOICE_TAG && _VR_STORAGE_TYPE
U32 *hCSpotterSD;
#endif
U8 VRState;
U8 VR_FuncType = VR_SI_Func;
U8 VR_GroupState = 0;

#if	_VR_GET_SCORE
S32 CmdId[10]={-100,-100,-100,-100,-100,-100,-100,-100,-100,-100};
S32 Score[10]={-100,-100,-100,-100,-100,-100,-100,-100,-100,-100};
#endif
#if	_VR_HIT_SCORE
S8 VR_HitScore;
#endif


/* Dynamic Allocate RAM-------------------------------------------------------*/
#if _USE_HEAP_RAM
static U8 *VRRam=NULL;
#else
	#if (_VR_ENGINE_RAM_SIZE >= _VR_SD_ENGINE_RAM_SIZE)
		static U8 VRRam[_VR_RAM_SIZE];
	#else 
		static U8 VRRam[_VR_SD_RAM_SIZE];
	#endif 
#endif

/* Extern Functions ----------------------------------------------------------*/
extern void CB_VR_ApiPresetting(void);
extern S8 CB_VR_VoiceTag_CreateOrDelete(U8 input);
extern void CB_VR_TrainTag_PromptAction_BeforeTrainingStart(void);
extern void CB_VR_TrainTag_PromptAction_BeforeSayingTag(void);


//------------------------------------------------------------------//
// Allocate VR memory
// Parameter: 
//          func: Choose Allocate function
// return value:
//          NONE
//------------------------------------------------------------------// 
#if _USE_HEAP_RAM
static void VR_AllocateMemory(U16 func)
{
    if(VRRam==NULL && (func&MEMORY_ALLOCATE_VR))    //VRRam
    {
		if(VR_FuncType == VR_SI_Func)
		#if _XIP_COC && _VR_STORAGE_TYPE
			VRRam = (U8 *)MemAlloc(COC_VR_RamSize());
		#else 
			VRRam = (U8 *)MemAlloc(_VR_RAM_SIZE);
		#endif
	#if _VR_VOICE_TAG && _VR_STORAGE_TYPE
		else if(VR_FuncType == VR_SD_Func)
			VRRam = (U8 *)MemAlloc(_VR_SD_RAM_SIZE);
	#endif
    }
    //else
    //	dprintf("Allocate twice. Ignore...\r\n");
}
//------------------------------------------------------------------//
// Free VR memory
// Parameter: 
//          func: Choose Allocate function
// return value:
//          NONE
//------------------------------------------------------------------// 
static void VR_FreeMemory(U16 func)
{
    if(VRRam!=NULL && (func&MEMORY_ALLOCATE_VR))    
    {
        MemFree(VRRam);
        VRRam=NULL;
        //dprintf("VR Free Mem\r\n");
    }
    //else
    //{
    //    dprintf("VR Already Free\r\n");
    //}
}
#endif


#if _VR_STORAGE_TYPE
static U32 SpiReadAddr = 0;
static U32 SpiBuf;
#endif

#define SPI_ADDR_BYTE               3                           
#define SPI_DATA_NUM_OFFSET         16

U32 SPIMgr_Read(U8 *dest, U8 *src, U32 nSize)
{
#if _VR_STORAGE_TYPE    
	if (((U32)src == SpiReadAddr) && (nSize == 4)){
		*(U32 *)dest = SpiBuf;
	}
	else {
		if (VR_FuncType == VR_SD_Func){
			P_WDT->CLR = C_WDT_CLR_Data;
		}
		
#define spiReadEZ	0
#if spiReadEZ == 1		
		SPI_FastReadStart((U32)src);
#else
		GPIOB->Data&=~(1<<0);
		
		SPI_TypeDef *SPI = P_SPI0;
		
	#if _SPI_ACCESS_MODE==SPI_1_1_1_MODE
		SPI->Data=SPI_READ_CMD;
	#elif _SPI_ACCESS_MODE==SPI_1_2_2_MODE    
		SPI->Data=SPI_2READ_CMD;
	#elif _SPI_ACCESS_MODE==SPI_1_1_2_MODE
		SPI->Data=SPI_DUAL_READ_CMD;
	#elif _SPI_ACCESS_MODE==SPI_1_4_4_MODE
		SPI->Data=SPI_4READ_CMD;
	#endif		
		
		SPI->Ctrl &= (~(C_SPI_FIFO_Length | C_SPI_DataMode));
		SPI->Ctrl |= C_SPI_Tx_Start | SPI_DATA_1;
		
		do{
		}while(SPI->Ctrl&C_SPI_Tx_Busy);
	   
		SPI->Data=(((U32)src&0xFF)<<16)|((U32)src&0xFF00)|((U32)src>>16);
		SPI->Ctrl &= (~(C_SPI_FIFO_Length | C_SPI_DataMode));
	#if _SPI_ACCESS_MODE==SPI_1_1_1_MODE
		SPI->Ctrl |= (C_SPI_Tx_Start |((SPI_ADDR_BYTE+0-1)<<SPI_DATA_NUM_OFFSET)| SPI_DATA_1); 
	#elif _SPI_ACCESS_MODE==SPI_1_2_2_MODE    
		SPI->Ctrl |= (C_SPI_Tx_Start |((SPI_ADDR_BYTE+1-1)<<SPI_DATA_NUM_OFFSET)| SPI_DATA_2); 
	#elif _SPI_ACCESS_MODE==SPI_1_1_2_MODE
		SPI->Ctrl |= (C_SPI_Tx_Start |((SPI_ADDR_BYTE+1-1)<<SPI_DATA_NUM_OFFSET)| SPI_DATA_1); 
	#elif _SPI_ACCESS_MODE==SPI_1_4_4_MODE
		SPI->Ctrl |= (C_SPI_Tx_Start |((SPI_ADDR_BYTE+3-1)<<SPI_DATA_NUM_OFFSET)| SPI_DATA_4); 
	#endif		

		do{
		}while(SPI->Ctrl&C_SPI_Tx_Busy);
		
		SPI->Ctrl &= (~(C_SPI_DataMode));
	#if _SPI_ACCESS_MODE==SPI_1_1_1_MODE
		SPI->Ctrl |= SPI_DATA_1;
	#elif _SPI_ACCESS_MODE==SPI_1_2_2_MODE    
		SPI->Ctrl |= SPI_DATA_2;
	#elif _SPI_ACCESS_MODE==SPI_1_1_2_MODE
		SPI->Ctrl |= SPI_DATA_2;
	#elif _SPI_ACCESS_MODE==SPI_1_4_4_MODE
		SPI->Ctrl |= SPI_DATA_4;
	#endif		
#endif
		SPI_FastRead((U32 *)dest, nSize);
		GPIOB->Data|=(1<<0);
		
		if (nSize == 4) {
			SpiReadAddr = (U32)src;
			SpiBuf = *(U32 *)dest;
		}
	}
#endif
    return 0;
}

U32 SPIMgr_ReadNoWait(U8 *dest, U8 *src, U32 nSize)
{
#if _VR_STORAGE_TYPE   
	SPIMgr_Read(dest, src, nSize);
#endif
    return 0;
}

U32 SPIMgr_Erase(U8 *dest, U32 nSize)
{
#if _VR_STORAGE_TYPE 
	//dprintf("E 0x%x size %d\r\n",(U32)dest, nSize);
	SPI_BurstErase_Sector((U32)dest, nSize);
#endif
	return 0;
}

U32 SPIMgr_Write(U8 *dest, U8 *src, U32 nSize)
{
#if _VR_STORAGE_TYPE 
	//dprintf("Write 0x%x size %d\r\n",(U32)dest, nSize);
	SPI_BurstWrite(src, (U32)dest, nSize);
#endif
	return 0;
}

//------------------------------------------------------------------//
// Adc ISR
// Parameter: 
//          size: data-buffer fill size 
//       pdest32: data-buffer address
// return value:
//          NONE
//------------------------------------------------------------------// 
void VR_AdcIsr(U8 size, U32 *pdest32){
    U8 i;
    S16 temp,*buf;
    
    if(VRState==VR_ON){
        if(!BufToggle){
            buf=Buf0+BufIdx;
        }else{
            buf=Buf1+BufIdx;
        }        
        
        for(i=0;i<size;i++){
            temp=((*pdest32++)-32768);//ADC data is unsigned integer. convert it to signed integer.
            
            *buf++=temp;
        }   
        BufIdx+= size;
        
        if(BufIdx==BUF_SIZE){
            if(BufToggle){
                BufReady=BUF1_READY;
            }else{
                BufReady=BUF0_READY;
            }        
            BufToggle^=1;
            BufIdx=0;
        }    
    }
}
//------------------------------------------------------------------//
// Set command model
// Parameter: 
//          cybase: cybase address
//   		command_model: model array address
// return value:
//          NONE
//------------------------------------------------------------------//     
static void VR_Init(U8 *cybase, U8 **command_model){
    U32 n;
	S32 nErr;

#if _VR_STORAGE_TYPE == 0    

	n = CSpotter16_GetMemoryUsage_Multi(cybase, command_model, numGroup, MAX_TIME);
	//dprintf("OTP_Mem_M= %d\r\n", n);
	if(n>_VR_ENGINE_RAM_SIZE){
		while(1);
	}    	
	hCSpotter = (U32)CSpotter16_Init_Multi(cybase, command_model, numGroup, MAX_TIME, (U8 *)(VRRam+ENGINE_BUF_OFFSET), _VR_ENGINE_RAM_SIZE, (U8 *)(VRStateRam), VR_STATE_SIZE, &nErr);
	
#else
	
	U8 i = 0;
	U8 *modelplus[_VR_COMBINED_GROUP_NUM];

#if _XIP_COC
	U16 engineramsize = COC_VR_EngineRamSize();
#else 
	U16 engineramsize = _VR_ENGINE_RAM_SIZE;
#endif

	for(i = 0; i <numGroup; i++)
	{
		modelplus[i] = *(command_model + i) + SpiOffset();
	}
	
	n = CSpotter16_GetMemoryUsage_Multi_FromSPI(cybase, modelplus, numGroup, MAX_TIME);
	//dprintf("SPI_Mem_M= %d\r\n", n);
	if(n>engineramsize){
		while(1);
	}    	
	hCSpotter = (U32)CSpotter16_Init_Multi_FromSPI(cybase, modelplus, numGroup, MAX_TIME, (U8 *)(VRRam+ENGINE_BUF_OFFSET), engineramsize, (U8 *)(VRStateRam), VR_STATE_SIZE, &nErr);
	
#endif

#if	_VR_GET_SCORE
	CSpotter16_SetEnableNBest((HANDLE)hCSpotter, ENABLE);
#endif

	CSpotter16_Reset((HANDLE)hCSpotter);
}    
//------------------------------------------------------------------//
// Voice Recogniztion start
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//     
static void VR_Start_(void){   
    U8 sts;
    
    if(VRState==VR_ON){
        VR_Stop();        
    }    
    
	sts = AUDIO_GetStatus_All();

    if(sts != STATUS_STOP){
        VRState=VR_HALT;
        return;
    }        
#if _USE_HEAP_RAM
	else
	{
		AUDIO_ServiceLoop();
	}
    VR_AllocateMemory(MEMORY_ALLOCATE_VR);
#endif
	if(firstTimeFlag == 0)
	{
		memset((void *)VRStateRam, 0x0, VR_STATE_SIZE);
		firstTimeFlag = 1;
    }
	
    Buf0=(S16 *)(VRRam+BUF0_OFFSET);
    Buf1=(S16 *)(VRRam+BUF1_OFFSET);
    
    BufIdx=0;
    BufToggle=0;
    BufReady=BUF_NOTREADY;
    
    InitAdc(_VR_SAMPLE_RATE);

#if _VR_VAD || _VR_VAD_TIMEOUT_EXTENSION
    VadInit();
#endif
#if _VR_UNKNOWN_COMMAND
    SD_Init();
#endif

    VR_Init(CyBaseAddr,ModelAddr);

	CB_VR_ApiPresetting();
	
    VRState=VR_ON;
    ADC_Cmd(ENABLE);
}   
//------------------------------------------------------------------//
// Voice Recogniztion start
// Parameter: 
//          cybase:cybase table address
//          model:model array address
// return value:
//          NONE
//------------------------------------------------------------------//     
void VR_Start(U8 *cybase, U8 **model){
#if _VR_STORAGE_TYPE	//VR command from SPI      
	
    CyBaseAddr = (U8*)(cybase + SpiOffset());
    ModelAddr = model;
	
#else					//VR command from OTP
	CyBaseAddr = cybase;
    ModelAddr = model;

#endif

    VR_Start_();    
}
 
//------------------------------------------------------------------//
// Voice Recogniztion halt
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//     
void VR_Halt(void){
    
    if(VRState==VR_ON){
		if(VR_FuncType == VR_SI_Func){		
			VR_ApiRelease(VR_SI_Func);
		#if _USE_HEAP_RAM
			VR_FreeMemory(MEMORY_ALLOCATE_VR);   
		#endif					
		}
        ADC_Cmd(DISABLE);
        VRState=VR_HALT;
    }
}    
//------------------------------------------------------------------//
// Voice Recogniztion stop
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//     
void VR_Stop(void){
    
    if(VRState==VR_ON){    
		VR_ApiRelease(VR_SI_Func);
	#if _USE_HEAP_RAM
        VR_FreeMemory(MEMORY_ALLOCATE_VR);   
	#endif    		
        ADC_Cmd(DISABLE);
    }
    VRState=VR_OFF;
}  

//------------------------------------------------------------------//
// VR Service loop
// Parameter: 
//          NONE
// return value:
//          -1: no command
//			-2: unknown command when _VR_UNKNOWN_COMMAND is set ENABLE
//      others: command index  
//------------------------------------------------------------------//     
S8 VR_ServiceLoop(void){

#if _VR_UNKNOWN_COMMAND
	S32 sts;
	static S8 id;
	static U32 time_getId = 0, time_getSD = 0;   
#if	_VR_HIT_SCORE
	static S8 tempHitScore;
#endif	 
#else
    S32 sts;
    S8 id;
#endif
	
    if(VRState==VR_ON){
        sts=1;
        if(BufReady==BUF0_READY)
        {
            BufReady=BUF_NOTREADY;
            //process buf0 here
#if _VR_VAD || _VR_VAD_TIMEOUT_EXTENSION
            VadProcess(Buf0,BUF_SIZE);
#endif
#if _VR_UNKNOWN_COMMAND
            SD_Process(Buf0,BUF_SIZE);
#endif
            sts=CSpotter16_AddSample((HANDLE)hCSpotter, Buf0, BUF_SIZE);			
        }
        else if(BufReady==BUF1_READY)
        {
            BufReady=BUF_NOTREADY;
            //process buf1 here
#if _VR_VAD || _VR_VAD_TIMEOUT_EXTENSION
            VadProcess(Buf1,BUF_SIZE);	
#endif
#if _VR_UNKNOWN_COMMAND
            SD_Process(Buf1,BUF_SIZE);
#endif
            sts=CSpotter16_AddSample((HANDLE)hCSpotter, Buf1, BUF_SIZE);				
        } 

	#if _VR_UNKNOWN_COMMAND
	#if _VR_STORAGE_TYPE
		if((SysTick - time_getId) > XIP_VR_UnknownCmd_Parameters(0)){
	#else
		if((SysTick - time_getId) > VR_ID_HOLD_TIME){
	#endif
		#if	_VR_HIT_SCORE
			tempHitScore = -1;
		#endif
			id = -1;
		}
				
		if(sts==0){
			id = CSpotter16_GetResult((HANDLE)hCSpotter);
		#if	_VR_GET_SCORE
			CSpotter16_GetNBestScore((HANDLE)hCSpotter, CmdId, Score, 10);
		#endif	
		#if	_VR_HIT_SCORE
			tempHitScore = CSpotter16_GetResultScore((HANDLE)hCSpotter);
		#endif
			
			CSpotter16_Reset((HANDLE)hCSpotter);
			
			time_getId = SysTick;
			//dprintf("id = %d, Time = %d\r\n", id, SysTick);
		}
	
		if(SD_GetSts()){
			time_getSD = SysTick;
			//dprintf("SD, Time = %d\r\n", SysTick);
		}
			
		if(time_getSD){
			if(id >= 0){
				time_getSD = 0;
			#if	_VR_HIT_SCORE
				VR_HitScore = tempHitScore;
			#endif	
				return id;
				
		#if _VR_STORAGE_TYPE
			}else if((SysTick - time_getSD) > XIP_VR_UnknownCmd_Parameters(1)) {
		#else
			}else if((SysTick - time_getSD) > VR_DELAY_SD_MAX_TIME) {
		#endif			
				time_getSD = 0;	
			#if	_VR_HIT_SCORE
				VR_HitScore = -1;
			#endif				
				return -2;
			}
			
		#if	_VR_HIT_SCORE
			VR_HitScore = -1;
		#endif
			return -1;
			
		}else{
		#if	_VR_HIT_SCORE
			VR_HitScore = -1;
		#endif
            return -1;
		}

	#else 
		
        if(sts==0){
            id = CSpotter16_GetResult((HANDLE)hCSpotter);
		#if	_VR_GET_SCORE
			CSpotter16_GetNBestScore((HANDLE)hCSpotter, CmdId, Score, 10);
		#endif	
		#if	_VR_HIT_SCORE
			VR_HitScore = CSpotter16_GetResultScore((HANDLE)hCSpotter);
		#endif
			
			CSpotter16_Reset((HANDLE)hCSpotter);
		
		#if _VR_VAD
            if(VR_GetVadSts2()==0) {
                //dprintf("Vad reject, id = %d\r\n", id);
                id = -1;
			#if	_VR_HIT_SCORE
				VR_HitScore = -1;
			#endif				
            }
		#endif     
            return id;	
			
        }else{
		#if	_VR_HIT_SCORE
			VR_HitScore = -1;
		#endif
            return -1;
        }
	#endif
		
    }else if(VRState==VR_HALT){

		sts = AUDIO_GetStatus_All();	
        if(sts == STATUS_STOP){
            VR_Start_();
        }    
    }  
    return -1;  
}

//------------------------------------------------------------------//
// Set GroupState and Number of Groups
// Parameter: 
//          groupState: VR_TRIGGER_STATE, VR_CMD_STATE_1, VR_CMD_STATE_2, ... VR_CMD_STATE_10
//          numModel: number of groups, value is 1 ~ _VR_COMBINED_GROUP_NUM
// return value:
//          NONE
//------------------------------------------------------------------//  
void VR_SetGroupState(U8 groupState, U8 numModel)
{
	VR_GroupState = groupState;
	numGroup = numModel;
}

//------------------------------------------------------------------//
// Get the number of commands from the model
// Parameter: 
//          model_addr:cybase table address
// return value:
//          0: no any command
//			>0: number of commands 
//------------------------------------------------------------------//  
U8 VR_GetNumWord(U8* model_addr)
{
#if _VR_STORAGE_TYPE == 0
	S32 nNum = CSpotter16_GetNumWord(model_addr);
#else
	S32 nNum = CSpotter16_GetNumWord_FromSPI(model_addr);
#endif

	if (nNum > 0)
		return nNum;
	else 
		return 0;
}

//------------------------------------------------------------------//
// Release the resource of Cyberon API
// Parameter: 
//          vrfunctype: VR function, VR_SI_Func or VR_SD_Func
// return value:
//          NONE
//------------------------------------------------------------------//  
void VR_ApiRelease(U8 vrfunctype)
{
	if ((vrfunctype == VR_SI_Func) && (hCSpotter != (U32)NULL)) {
		CSpotter16_Release((HANDLE)hCSpotter);
		hCSpotter = (U32)NULL;
	}
#if _VR_VOICE_TAG && _VR_STORAGE_TYPE
	else if ((vrfunctype == VR_SD_Func) && (hCSpotterSD != NULL)) {
		CSpotterSD16_Release(hCSpotterSD);
		hCSpotterSD = NULL;
	}
#endif		
		
}

#if _VR_VOICE_TAG && _VR_STORAGE_TYPE
//------------------------------------------------------------------//
// Delete a voice tag from the model
// Parameter: 
//          lpbySDModel: SD model address
//          nModelSize: SD model size
//          nTagID: the voice tad ID
// return value:
//          0: success
//		   	<0: error code
//------------------------------------------------------------------// 
S32 DoVR_DelTag_SPI(U8* lpbySDModel, U32 nModelSize, U32 nTagID) {
	
	U32 nSize;
	S32 nErr;

	if ((nErr = CSpotterSD16_DeleteWord_FromSPI(hCSpotterSD, lpbySDModel, nModelSize, nTagID, &nSize)) != CSPOTTER_SUCCESS )
		return nErr;
	//dprintf("nSize= %d\r\n", nSize);
	return nSize;
}

//------------------------------------------------------------------//
// Train a voice tag into the model
// Parameter: 
//          lpbySDModel: SD model address
//          nModelSize: SD model size
//			lpbySDBuffer: SD training Buffer address
//			nSDBufSize: SD training Buffer size
//          nTagID: the voice tad ID
//			nTimeOut: time out timer
// return value:
//		   	<0: error code
//     		>0: total voice tag size
//------------------------------------------------------------------// 
S32 DoVR_TrainTag_SPI(U8* lpbySDModel, U32 nModelSize, U8* lpbySDBuffer, U32 nSDBufSize, U32 nTagID, U32 nTimeOut) {

	if (NUM_UTTR * UTTR_BUF_SIZE > nSDBufSize + UTTR_BUF_SIZE)
		while(1);
	
	S32 nErr = 0;
	U32 nWait, nSize;
	U8 i;

	CB_VR_TrainTag_PromptAction_BeforeTrainingStart();
	
	// Start training
	for (i = 0; i < NUM_UTTR; i++) {

		CB_VR_TrainTag_PromptAction_BeforeSayingTag();
	
		if (i == (NUM_UTTR-1))
			break;
	
		if ((nErr = CSpotterSD16_AddUttrStart_FromSPI(hCSpotterSD, i, lpbySDBuffer + (i * UTTR_BUF_SIZE), UTTR_BUF_SIZE)) != CSPOTTER_SUCCESS)
		{
			return nErr;
		}	

		Buf0=(S16 *)(VRRam+BUF0_OFFSET);
		Buf1=(S16 *)(VRRam+BUF1_OFFSET);
		
		nWait = 0;
		BufIdx = 0;
		BufToggle = 0;
		BufReady = BUF_NOTREADY;
		InitAdc(_VR_SAMPLE_RATE);
		VRState=VR_ON;
    	ADC_Cmd(ENABLE);
		
		while (1) {
			
			if (BufReady == BUF_NOTREADY) {
				continue;
			}
			
			else if (BufReady == BUF0_READY) {
				BufReady = BUF_NOTREADY;
				//process buf0 here
				if ((nErr = CSpotterSD16_AddSample_FromSPI(hCSpotterSD, Buf0, BUF_SIZE)) != CSPOTTER_ERR_NeedMoreSample) {
					break;
					}
			}
			
			else if (BufReady == BUF1_READY) {
				BufReady = BUF_NOTREADY;
				//process buf1 here
				if ((nErr = CSpotterSD16_AddSample_FromSPI(hCSpotterSD, Buf1, BUF_SIZE)) != CSPOTTER_ERR_NeedMoreSample){
					break;
					}
			}

			nWait++;
			if (nWait >= nTimeOut)
				return nErr;
			
			WDT_Clear();
		}

		if ((nErr = CSpotterSD16_AddUttrEnd_FromSPI(hCSpotterSD)) != CSPOTTER_SUCCESS)
			return nErr;
		
		//U32 nStart, nEnd;
		//CSpotterSD16_GetUttrEPD(hCSpotterSD, &nStart, &nEnd);
		//dprintf("Uttr %d: %d msec\r\n", i, (nEnd-nStart)/16);
		
	}
	
	ADC_Cmd(DISABLE);
	
	if ((nErr = CSpotterSD16_TrainWord_FromSPI(hCSpotterSD, lpbySDModel, nModelSize, nTagID, 1, &nSize)) != CSPOTTER_SUCCESS)
		return nErr;
	
	return nSize;
}

//------------------------------------------------------------------//
// Flow of Training/Deleting a voice tag
// Parameter: 
//			input: Tag input data
// return value:
//          DO_NOTHING or nReturnCode
//------------------------------------------------------------------// 
S8 VR_Train(U8 input) {
	
	S32 nErr;
	S8 nReturnCode = AUDIO_GetStatus_All();
    if(nReturnCode != STATUS_STOP){
        return DO_NOTHING;
    } 	
	
	VR_ApiRelease(VR_SI_Func);
	VR_FuncType = VR_SD_Func;
	
	U8* lpbyBaseModel = (U8*)VR_SPI_GroupAddr(0) + SpiOffset();
    
	// Training_Engine initial
	U32 n = CSpotterSD16_GetMemoryUsage_FromSPI(lpbyBaseModel, 0);
	//dprintf("SDMem:%d\r\n", n);
	if (n > _VR_SD_ENGINE_RAM_SIZE)
		while(1);
	
#if _USE_HEAP_RAM	
	VR_FreeMemory(MEMORY_ALLOCATE_VR); 
	VR_AllocateMemory(MEMORY_ALLOCATE_VR);
#endif
	
	hCSpotterSD = CSpotterSD16_Init_FromSPI(lpbyBaseModel, (VRRam+ENGINE_BUF_OFFSET), _VR_SD_ENGINE_RAM_SIZE, &nErr);

	nReturnCode = CB_VR_VoiceTag_CreateOrDelete(input);

	VR_ApiRelease(VR_SD_Func);
	VR_FuncType = VR_SI_Func;
#if _USE_HEAP_RAM	
	VR_FreeMemory(MEMORY_ALLOCATE_VR);
#endif

	return nReturnCode;
}
#endif
#if _VR_STORAGE_TYPE
//------------------------------------------------------------------//
// Get VR group address from SPI Flash
// Parameter: 
//          groupindex: 0 = cybase; 
//          			others = VR group 
// return value:
//          0: error address
//			>0: VR group address 
//------------------------------------------------------------------//  
U32 VR_SPI_GroupAddr(U8 groupindex)
{
	U8 tempbuf[4];
	U8 numberOfBin;
	U32 ref_addr, start_addr;

	ref_addr = RESOURCE_GetAddress(RESOURCE_GetResourceCnt()-1);
	
	SPI_BurstRead(tempbuf, ref_addr, 4);
	numberOfBin = tempbuf[0];	//|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24); 
	
	start_addr = ref_addr + (numberOfBin+1)*4 - SpiOffset();
	
	if(groupindex < numberOfBin) {
		if(groupindex == 0) {
			return start_addr;
		} else {
			U8 i;
			U32 temp = 0;
			for(i = 1; i <= groupindex; i++) {
				SPI_BurstRead(tempbuf, ref_addr + i*4, 4);
				temp += (tempbuf[0]|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24)); 
			}
			return temp + start_addr;
		}		
	}
	return 0;
}

U32 VR_SPI_CVRGroupAddr(U8 CVRindex, U8 groupindex)
{
	U8 tempbuf[4];
	U8 numberOfBin, i;
	U32 ref_addr, start_addr;

	tempbuf[0] = RESOURCE_GetResourceCnt();
	for(i = 0; i < tempbuf[0]; i++){
		if(RESOURCE_GetType(i) == 1){
			break;
		}
	}
	
	ref_addr = RESOURCE_GetAddress(CVRindex + i);
	
	SPI_BurstRead(tempbuf, ref_addr, 4);
	numberOfBin = tempbuf[0];	//|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24); 
	
	start_addr = ref_addr + (numberOfBin+1)*4 - SpiOffset();
	
	if(groupindex < numberOfBin) {
		if(groupindex == 0) {
			return start_addr;
		} else {
			//U8 i;
			U32 temp = 0;
			for(i = 1; i <= groupindex; i++) {
				SPI_BurstRead(tempbuf, ref_addr + i*4, 4);
				temp += (tempbuf[0]|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24)); 
			}
			return temp + start_addr;
		}		
	}
	return 0;
}

#endif
#endif
