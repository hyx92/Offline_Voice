/* Includes ------------------------------------------------------------------*/
#include "include.h"
#include "nx1_gpio.h"
#include "IoTab.h"
#include "nx1_def.h"
#include "nx1_timer.h"
#include "nx1_spi.h"
/* Defines -------------------------------------------------------------------*/
#if _ACTION_MODULE

#define E_VIO_INVALID_FILE_IDX          1
#define E_VIO_INVALID_ACTION_IDX        2
#define E_VIO_INVALID_CHANNEL           3

#define VIO_FLAG_REPEAT                 0x1
#define VIO_FLAG_HIGH_BUSY              0x2
#define VIO_FLAG_LOW_BUSY               0x4

//#define VIO_STATE_HIGH                  0
//#define VIO_STATE_LOW                   1
//#define VIO_STATE_TOGGLE_HIGH           2
//#define VIO_STATE_TOGGLE_LOW            3

#define VIO_HIGH_BUSY_OFFSET            0x1
#define VIO_LOW_BUSY_OFFSET             0x2

#define VIO_START 
#define VIO_POSITIVE_SLOPE	0x01
#define VIO_NEGATIVE_SLOPE 	0x02
#define VIO_HORIZONTAL 		0x04
#define VIO_MARK			0x08
#define VIO_END 			0x10

#define BUFFER_NONE         0x00
#define BUFFER_START        0x01
#define BUFFER_MIDDLE       0x02
//
#define VIO_FRAME_TOTAL_TICK            (1000000/_NYIDE_TIMEBASE/_NYIDE_FRAMERATE)
#define VIO_FRAME_TOTAL_TICKwPOINT		((256000000+(_NYIDE_TIMEBASE*_NYIDE_FRAMERATE)/2)/(_NYIDE_TIMEBASE*_NYIDE_FRAMERATE))

#define PIN_MASK    0x0F
#define PORT_MASK   0xF0
#define VIO_END_FLAG        0x8000
#define VIO_MARK_FLAG       0x4000
#define VIO_END_OUT			0x2000

#define END_FLAG_CHECK		0x01
#define END_FLAG_FINAL_OUT	0x02
typedef struct{
    U16 target_duty_on;
    U16 cur_duty_on;
    //U32 total_frame;
    U32 cur_frame;
    U32 start_data_addr;
    U32 cur_data_addr;
    //U32 file;
    U16 diff_duty_on;
    U8  fraction_duty_on;
    U8  fraction_duty_off;
    U8  line_type;
    U8  storage_mode;
    U8  play_mode;
    U8  mark_flag;
    U8  end_flag;
}ACTION_DATA_CTRL;


/* Static Variables ----------------------------------------------------------*/
volatile ACTION_IO_CTRL  Action_io_ctrl[_VIO_CH_NUM];
static ACTION_DATA_CTRL Action_data_ctrl[_VIO_CH_NUM];
//static int test_cnt;



/* Extern  ----------------------------------------------------------*/
extern volatile S8  g_semaphore;



//------------------------------------------------------------------//
// Get data from storage
// Parameter: 
//          ch:vio channel
//          mode: OTP_MODE, SPI_MODE
//          len : the lenght of getting data
//          first_flag : first node
//          file : io action file
//          vio_data : storage data 
// return value:
//          NONE
//------------------------------------------------------------------//
void GetStorageData(U8 ch, U8 mode, U8 len, U8 first_flag, U32 file, U8 *vio_data)
{
    int i;
    if(mode==OTP_MODE)
    {
        if(first_flag)
        {
            Action_data_ctrl[ch].cur_data_addr = file;
            Action_data_ctrl[ch].start_data_addr = file;
            //dprintf("OTP Addr 0x%x\r\n",Action_data_ctrl[ch].cur_data_addr );
        }
        for(i=0;i<len;i++)
        {
            vio_data[i]=*((U8 *)Action_data_ctrl[ch].cur_data_addr);
            Action_data_ctrl[ch].cur_data_addr++;
        }
    }
    else //SPI_MODE
    {
        U8  tempbuf[12];
        int i;
        
        if(first_flag)
        {
            
            U32 addr;
			
			if(file&ADDRESS_MODE){							// Action doesn't support ACTION_REPEAT on ADDRESS_MODE
				addr=file&(~ADDRESS_MODE);
			}else{            
				addr=SpiOffset();
				SPI_BurstRead(tempbuf,addr+SPI_BASE_INFO_SIZE+(SPI_TAB_INFO_ENTRY_SIZE*file),4);
				addr+=(tempbuf[0]|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24));
				addr&=(~SPI_DATA_TYPE_MASK);
            }
			
            Action_data_ctrl[ch].cur_data_addr = addr;
            Action_data_ctrl[ch].start_data_addr = file;
            //dprintf("SPI Addr 0x%x\r\n",Action_data_ctrl[ch].cur_data_addr);
            
        }
        
        SPI_BurstRead(tempbuf,Action_data_ctrl[ch].cur_data_addr,len); //read data from SPI
        for(i=0;i<len;i++)
        {
            vio_data[i] = tempbuf[i];
        }
        Action_data_ctrl[ch].cur_data_addr += len;                
    }
}
//------------------------------------------------------------------//
// Get data from storage
// Parameter: 
//          ch:vio channel
//          vio_data : storage data 
// return value:
//          None
//------------------------------------------------------------------//
void ParsingData(U8 ch, U8 *vio_data)
{
    Action_data_ctrl[ch].cur_frame = 0;
    //Action_data_ctrl[ch].target_duty_on = 0;
    Action_data_ctrl[ch].fraction_duty_on = Action_data_ctrl[ch].fraction_duty_off = 0;
    Action_data_ctrl[ch].line_type      = 0;
    Action_data_ctrl[ch].diff_duty_on   = 0;
    //Action_data_ctrl[ch].total_frame    = 0;

    if(vio_data[0]&VIO_POSITIVE_SLOPE)
    {
        Action_data_ctrl[ch].line_type = VIO_POSITIVE_SLOPE;
        Action_data_ctrl[ch].diff_duty_on = vio_data[3] + (vio_data[4]<<8);
        Action_data_ctrl[ch].cur_frame  = vio_data[5];
    }
    else if(vio_data[0]&VIO_NEGATIVE_SLOPE)
    {
        Action_data_ctrl[ch].line_type = VIO_NEGATIVE_SLOPE;
        Action_data_ctrl[ch].diff_duty_on = vio_data[3] + (vio_data[4]<<8);
        Action_data_ctrl[ch].cur_frame  = vio_data[5];
    }
    else if(vio_data[0]&VIO_HORIZONTAL)
    {
        Action_data_ctrl[ch].line_type = VIO_HORIZONTAL;
        Action_data_ctrl[ch].cur_frame  = vio_data[3] + (vio_data[4]<<8) + (vio_data[5]<<16);
        Action_data_ctrl[ch].cur_duty_on  = vio_data[1] + (vio_data[2]<<8);
    }
    else if(vio_data[0]&VIO_MARK)
    {
        Action_data_ctrl[ch].line_type = VIO_MARK;
        Action_data_ctrl[ch].cur_data_addr -= 6; //MARK only use 2 bytes (8-6)
        //Action_data_ctrl[ch].mark_flag = 1;
        Action_data_ctrl[ch].mark_flag = vio_data[1];
#if 0
		dprintf("======MARK_NO: %d\r\n",Action_data_ctrl[ch].mark_flag); 
#endif
        return;
    }
    else if(vio_data[0]&VIO_END)
    {
        Action_data_ctrl[ch].line_type = VIO_END;
		if(Action_data_ctrl[ch].target_duty_on==VIO_FRAME_TOTAL_TICKwPOINT)
		{
			Action_data_ctrl[ch].end_flag |= END_FLAG_FINAL_OUT;//1:100%
		}
		else
		{
			Action_data_ctrl[ch].end_flag &= ~END_FLAG_FINAL_OUT;//0:not 100%
		}
        return;
    }
    
    Action_data_ctrl[ch].target_duty_on = vio_data[1] + (vio_data[2]<<8);
    Action_data_ctrl[ch].fraction_duty_on = vio_data[6];
    Action_data_ctrl[ch].fraction_duty_off = vio_data[7];
    //Action_data_ctrl[ch].cur_frame = Action_data_ctrl[ch].total_frame;  //need check total frame including or not fraction.
#if 0
    dprintf("line_type %d\r\n",Action_data_ctrl[ch].line_type);
	//dprintf("total_frame %d\r\n",Action_data_ctrl[ch].total_frame);
    dprintf("diff_duty_on %d\r\n",Action_data_ctrl[ch].diff_duty_on);
    dprintf("target_duty_on %d\r\n",Action_data_ctrl[ch].target_duty_on);
    dprintf("fraction_duty_on %d\r\n",Action_data_ctrl[ch].fraction_duty_on);
    dprintf("fraction_duty_off %d\r\n",Action_data_ctrl[ch].fraction_duty_off);
    dprintf("cur_duty_on %d\r\n",Action_data_ctrl[ch].cur_duty_on); 
#endif
}
//------------------------------------------------------------------//
// Collect io data
// Parameter: 
//          ch:vio channel
//          vio_data : storage data 
// return value:
//          NONE
//------------------------------------------------------------------//
void CollectIoData(U8 ch, U8 len)
{
    int i=0;	//jhua

    if(Action_data_ctrl[ch].end_flag & END_FLAG_CHECK)   return;

    if(Action_io_ctrl[ch].wr_req == BUFFER_START)
    {
        i = 0;
    }
    else if(Action_io_ctrl[ch].wr_req == BUFFER_MIDDLE)
    {
        i = VIO_DATA_BUF_CNT>>1;
        //dprintf("CollectIO %d\r\n",i);
    }
    Action_io_ctrl[ch].wr_req = BUFFER_NONE;
    while(len--)
    {
        if(Action_data_ctrl[ch].cur_frame)
        {
            GIE_DISABLE();
            Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt = (Action_data_ctrl[ch].cur_duty_on>>8);
            Action_io_ctrl[ch].frame_cnt[i].low_duty_cnt  = VIO_FRAME_TOTAL_TICK - Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt;

            //if(Action_data_ctrl[ch].mark_flag)  Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_MARK_FLAG;
            if(Action_data_ctrl[ch].mark_flag)
			{
				Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_MARK_FLAG;
				Action_io_ctrl[ch].frame_cnt[i].low_duty_cnt  |= Action_data_ctrl[ch].mark_flag<<8;
			}
            GIE_ENABLE();
            
            Action_data_ctrl[ch].mark_flag = 0;
            if(Action_data_ctrl[ch].line_type == VIO_NEGATIVE_SLOPE)
            {
                if(Action_data_ctrl[ch].cur_duty_on < Action_data_ctrl[ch].diff_duty_on)
                {
                    Action_data_ctrl[ch].cur_duty_on =0;
                }
                else
                {
                    Action_data_ctrl[ch].cur_duty_on -= Action_data_ctrl[ch].diff_duty_on;
                }
            }
            else
            {
                Action_data_ctrl[ch].cur_duty_on += Action_data_ctrl[ch].diff_duty_on;
            }
            //Action_data_ctrl[ch].cur_frame--;
            if(Action_data_ctrl[ch].cur_frame==1)//target point
            {
                Action_data_ctrl[ch].cur_duty_on = Action_data_ctrl[ch].target_duty_on;
            }
			Action_data_ctrl[ch].cur_frame--;
            //dprintf("ch=%d h_duty_cnt[%d] %d cur_frame %d\r\n",ch,i,Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt,Action_data_ctrl[ch].cur_frame);
        }
        else if(Action_data_ctrl[ch].fraction_duty_on || Action_data_ctrl[ch].fraction_duty_off )
        {
            GIE_DISABLE();
            Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt = Action_data_ctrl[ch].fraction_duty_on;
            Action_io_ctrl[ch].frame_cnt[i].low_duty_cnt  = Action_data_ctrl[ch].fraction_duty_off;

            //if(Action_data_ctrl[ch].mark_flag)  Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_MARK_FLAG;
            if(Action_data_ctrl[ch].mark_flag)
			{
				Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_MARK_FLAG;
				Action_io_ctrl[ch].frame_cnt[i].low_duty_cnt  |= Action_data_ctrl[ch].mark_flag<<8;
			}
            GIE_ENABLE();
            Action_data_ctrl[ch].mark_flag = 0;

            //dprintf("ch %d [%d] f on %d off %d\r\n",ch,i,Action_data_ctrl[ch].fraction_duty_on,Action_data_ctrl[ch].fraction_duty_off);

            Action_data_ctrl[ch].fraction_duty_on = Action_data_ctrl[ch].fraction_duty_off = 0;
        }
        else // parsing next node
        {
            U8 vio_data[12];

            len++; // write back
            //dprintf("Next node \r\n");
            GetStorageData(ch, Action_data_ctrl[ch].storage_mode, 8, 0, 0, vio_data);
            ParsingData(ch,vio_data);
            if(Action_data_ctrl[ch].line_type==VIO_END)
            {
                if(Action_data_ctrl[ch].play_mode == VIO_FLAG_REPEAT)
                {
                    // Re-initial
                    //Action_data_ctrl[ch].cur_data_addr = Action_data_ctrl[ch].start_data_addr+2;
                    
                    //GetStorageData(ch, Action_data_ctrl[ch].storage_mode, 8, 0, 0, vio_data);
                    
                    //ParsingData(ch,vio_data);

                    /////////////////////////////////////////////////////////////////////////////////////////////

                    GetStorageData(ch, Action_data_ctrl[ch].storage_mode, 10, 1, Action_data_ctrl[ch].start_data_addr, vio_data);

                    //parsing data
                    Action_data_ctrl[ch].cur_duty_on = vio_data[0]+(vio_data[1]<<8); //initial value
                    //if(Action_data_ctrl[ch].line_type==NULL)
                    ParsingData(ch, &vio_data[2]); //too short action does not think here. < 4 frames
                    
                    ////////////////////////////////////////////////////////////////////////////////////////////////
                }
                else
                {
                    //end, how to end play
                    Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_END_FLAG;
					if(Action_data_ctrl[ch].end_flag & END_FLAG_FINAL_OUT)
					{
						Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt |= VIO_END_OUT;
					}
					else
					{
						Action_io_ctrl[ch].frame_cnt[i].high_duty_cnt &= ~VIO_END_OUT;
					}
                    //Action_data_ctrl[ch].end_flag = 1;
					Action_data_ctrl[ch].end_flag |= END_FLAG_CHECK;
                    len = 0; //end point, leave frame process
                    //dprintf("End P %d\r\n",i);
                }
            }
            i--; //write back
        }
        i++;        
    }
    
}
//------------------------------------------------------------------//
// Play IO
// Parameter: 
//          ch:vio channel, ACTION_CH0 ~ ACTION_CHx (x = _VIO_CH_NUM-1)
//          file: io action file
//          port: assign gpio port, port|pin
//          repeat:1:repeat, 0:no_repeat
//          busy: 0:busy_low,1:busy_high
//          mode: OTP_MODE, SPI_MODE
// return value:
//          0:ok, others:fail
//------------------------------------------------------------------//      
U8 ACTION_PlayIO(U8 ch,const U32 file,U8 port,U8 repeat,U8 busy,U8 mode)
{
    GPIO_TypeDef *p_port;
    U8 pin;
    U8 vio_data[12];
    U8 state;
    
	ch -= _AUDIO_API_CH_NUM;
    if(ch>=_VIO_CH_NUM)
    {
        return E_VIO_INVALID_CHANNEL;
    }  
    GIE_DISABLE();
    state = Action_io_ctrl[ch].state;
    Action_io_ctrl[ch].state            = STATUS_STOP;
    Action_data_ctrl[ch].storage_mode   = mode;
    Action_data_ctrl[ch].play_mode      = repeat;
//    Action_data_ctrl[ch].file           = file;
    Action_data_ctrl[ch].mark_flag      = 0;
    Action_data_ctrl[ch].end_flag       = 0;
    Action_io_ctrl[ch].port             = port;
    Action_io_ctrl[ch].busy_level       = busy;
    Action_io_ctrl[ch].wr_req           = BUFFER_NONE;
    Action_io_ctrl[ch].rd_index         = 0;
    Action_io_ctrl[ch].frame_cnt[0].high_duty_cnt = 0;
    Action_io_ctrl[ch].frame_cnt[0].low_duty_cnt  = 0;
    

    //test_cnt=0;


    // IO initial
    pin=port&PIN_MASK;
    port=port&PORT_MASK;
    if(port==ACTION_PORTA)
    {
        p_port=GPIOA;
    }
    else if(port==ACTION_PORTB)
    {
        p_port=GPIOB;
    }
    else
    {
        p_port=GPIOC;
    }            
    //GPIO_Write(p_port,pin,!Action_io_ctrl[ch].busy_level);
	/*if (Action_io_ctrl[ch].busy_level)
    {
		GPIO_Init(p_port,pin,GPIO_MODE_OUT_LOW);
	}
	else
	{
		GPIO_Init(p_port,pin,GPIO_MODE_OUT_HI);
	}*/
	p_port->MFP &= (~(1<<pin));
	p_port->DIR  &= (~(1<<pin));
	
    //
    GetStorageData(ch, mode, 10, 1, file, vio_data);

    //parsing data
    Action_data_ctrl[ch].cur_duty_on = vio_data[0]+(vio_data[1]<<8); //initial value
    //if(Action_data_ctrl[ch].line_type==NULL)
    ParsingData(ch, &vio_data[2]); //too short action does not think here. < 4 frames
    Action_io_ctrl[ch].wr_req = BUFFER_START;
    CollectIoData(ch, VIO_DATA_BUF_CNT);
    // start action
    Action_io_ctrl[ch].state = STATUS_PLAYING;
    
    // if any vio is going, do not enable again.
    if(g_semaphore==0)
    {
#if     (_VIO_TIMER==USE_TIMER0 && _ACTION_MODULE)
    	TIMER_Init(TIMER0,TIMER_CLKDIV_1,(OPTION_HIGH_FREQ_CLK/1000000*_NYIDE_TIMEBASE)-1);
    	TIMER_Cmd(TIMER0,ENABLE);
#elif   (_VIO_TIMER==USE_TIMER1 && _ACTION_MODULE)
    	TIMER_Init(TIMER1,TIMER_CLKDIV_1,(OPTION_HIGH_FREQ_CLK/1000000*_NYIDE_TIMEBASE)-1);
    	TIMER_Cmd(TIMER1,ENABLE);
#elif   (_VIO_TIMER==USE_TIMER2 && _ACTION_MODULE)
    	TIMER_Init(TIMER2,TIMER_CLKDIV_1,(OPTION_HIGH_FREQ_CLK/1000000*_NYIDE_TIMEBASE)-1);
    	TIMER_Cmd(TIMER2,ENABLE);
#elif   (_VIO_TIMER==USE_TIMER_PWMA && _ACTION_MODULE)
    	TIMER_Init(PWMATIMER,TIMER_CLKDIV_1,(OPTION_HIGH_FREQ_CLK/1000000*_NYIDE_TIMEBASE)-1);
    	TIMER_Cmd(PWMATIMER,ENABLE);
#elif   (_VIO_TIMER==USE_TIMER_PWMB && _ACTION_MODULE)
    	TIMER_Init(PWMBTIMER,TIMER_CLKDIV_1,(OPTION_HIGH_FREQ_CLK/1000000*_NYIDE_TIMEBASE)-1);
    	TIMER_Cmd(PWMBTIMER,ENABLE);
#else// RTC is default
#endif
	}
    if(state == STATUS_STOP)
    {
        g_semaphore++;
    }
    GIE_ENABLE();
    
    
    return 0;
    
}

//------------------------------------------------------------------//
// Pause IO
// Parameter: 
//          ch:vio channel, ACTION_CH0 ~ ACTION_CHx (x = _VIO_CH_NUM-1), ACTION_All_CH
// return value:
//          NONE
//------------------------------------------------------------------//      
void ACTION_PauseIO(U8 ch)
{
    U8 i;
	ch -= _AUDIO_API_CH_NUM;
    if(ch<_VIO_CH_NUM){
        if(Action_io_ctrl[ch].state==STATUS_PLAYING){
            Action_io_ctrl[ch].state=STATUS_PAUSE;
        }
    }else if(ch==(ACTION_All_CH - _AUDIO_API_CH_NUM)){
        for(i=0;i<_VIO_CH_NUM;i++){
            if(Action_io_ctrl[i].state==STATUS_PLAYING){
                Action_io_ctrl[i].state=STATUS_PAUSE;        
            }
        }
    }                
}   
//------------------------------------------------------------------//
// Resume IO
// Parameter: 
//          ch:vio channel, ACTION_CH0 ~ ACTION_CHx (x = _VIO_CH_NUM-1), ACTION_All_CH
// return value:
//          NONE
//------------------------------------------------------------------//      
void ACTION_ResumeIO(U8 ch)
{
    U8 i;
	ch -= _AUDIO_API_CH_NUM;
    if(ch<_VIO_CH_NUM){
        if(Action_io_ctrl[ch].state==STATUS_PAUSE){
            Action_io_ctrl[ch].state=STATUS_PLAYING;
        }      
    }else if(ch==(ACTION_All_CH - _AUDIO_API_CH_NUM)){
        for(i=0;i<_VIO_CH_NUM;i++){
            if(Action_io_ctrl[i].state==STATUS_PAUSE){
                Action_io_ctrl[i].state=STATUS_PLAYING;        
            }      
        }
    }          
}   
//------------------------------------------------------------------//
// Stop IO
// Parameter: 
//          ch:vio channel, ACTION_CH0 ~ ACTION_CHx (x = _VIO_CH_NUM-1), ACTION_All_CH
// return value:
//          NONE
//------------------------------------------------------------------//      
void ACTION_StopIO(U8 ch)
{
	U8 i,port,pin,state;
    GPIO_TypeDef *p_port;
    
	ch -= _AUDIO_API_CH_NUM;
    if(ch<_VIO_CH_NUM){
        
        state=Action_io_ctrl[ch].state;
        Action_io_ctrl[ch].state=STATUS_STOP;
        GIE_DISABLE();
        if(state!=STATUS_STOP)
        {
            g_semaphore--;
        }
        if(g_semaphore==0)
        {
#if		(_VIO_TIMER==USE_TIMER0 && _ACTION_MODULE)
            TIMER_Cmd(TIMER0,DISABLE);
#elif   (_VIO_TIMER==USE_TIMER1 && _ACTION_MODULE)
            TIMER_Cmd(TIMER1,DISABLE);
#elif   (_VIO_TIMER==USE_TIMER2 && _ACTION_MODULE)
            TIMER_Cmd(TIMER2,DISABLE);
#elif   (_VIO_TIMER==USE_TIMER_PWMA && _ACTION_MODULE)
            TIMER_Cmd(PWMATIMER,DISABLE);
#elif   (_VIO_TIMER==USE_TIMER_PWMB && _ACTION_MODULE)
            TIMER_Cmd(PWMBTIMER,DISABLE);
#endif      
        }
        else if(g_semaphore<0)
        {
            g_semaphore = 0;
        }
        GIE_ENABLE();
        //ClrWaitingAction(ch);
        
        port=Action_io_ctrl[ch].port&PORT_MASK;
        pin=Action_io_ctrl[ch].port&PIN_MASK;						// i is replaced by ch, //jhua
        if(port==ACTION_PORTA){
            p_port=GPIOA;
        }else if(port==ACTION_PORTB){
            p_port=GPIOB;
        }else{
            p_port=GPIOC;
        }   
        if(state==STATUS_PLAYING||state==STATUS_PAUSE){
            GPIO_Write(p_port,pin,!Action_io_ctrl[ch].busy_level);	// i is replaced by ch, //jhua
        }
    }     

    else if(ch==(ACTION_All_CH - _AUDIO_API_CH_NUM)){
        for(i=0;i<_VIO_CH_NUM;i++){

            state=Action_io_ctrl[i].state;
            Action_io_ctrl[i].state=STATUS_STOP;
            g_semaphore--;
            if(g_semaphore==0)
            {
	#if		(_VIO_TIMER==USE_TIMER0 && _ACTION_MODULE)
				TIMER_Cmd(TIMER0,DISABLE);
	#elif   (_VIO_TIMER==USE_TIMER1 && _ACTION_MODULE)
				TIMER_Cmd(TIMER1,DISABLE);
	#elif   (_VIO_TIMER==USE_TIMER2 && _ACTION_MODULE)
				TIMER_Cmd(TIMER2,DISABLE);
	#elif   (_VIO_TIMER==USE_TIMER_PWMA && _ACTION_MODULE)
				TIMER_Cmd(PWMATIMER,DISABLE);
	#elif   (_VIO_TIMER==USE_TIMER_PWMB && _ACTION_MODULE)
				TIMER_Cmd(PWMBTIMER,DISABLE);
	#endif           
            }
            else if(g_semaphore<0)
            {
                g_semaphore = 0;
            }
            //ClrWaitingAction(ch);

            port=Action_io_ctrl[i].port&PORT_MASK;
            pin=Action_io_ctrl[i].port&PIN_MASK;
            if(port==ACTION_PORTA){
                p_port=GPIOA;
            }else if(port==ACTION_PORTB){
                p_port=GPIOB;
            }else{
                p_port=GPIOC;
            }   
            if(state==STATUS_PLAYING||state==STATUS_PAUSE){
                GPIO_Write(p_port,pin,!Action_io_ctrl[i].busy_level);
            }
        }
    }	
    
}   
//------------------------------------------------------------------//
// Get IO current status
// Parameter: 
//          ch:vio channel, ACTION_CH0 ~ ACTION_CHx (x = _VIO_CH_NUM-1)
// return value:
//          status:STATUS_STOP,STATUS_PLAYING,STATUS_PAUSE
//------------------------------------------------------------------// 
U8 ACTION_GetIOStatus(U8 ch){
	ch -= _AUDIO_API_CH_NUM;
    if(ch>=_VIO_CH_NUM){
        return 0;
    }       
    return Action_io_ctrl[ch].state;
    //return 0;
} 
//------------------------------------------------------------------//
// Actino Service Loop
// Parameter: 
// return value:
//          0:ok, others:fail
//------------------------------------------------------------------//      
void ACTION_ServiceLoop(void)
{
    U8 i;
    
    for(i=0;i<_VIO_CH_NUM;i++)
    {
        if(Action_io_ctrl[i].wr_req != BUFFER_NONE && Action_io_ctrl[i].state==STATUS_PLAYING)
        {
            CollectIoData(i,VIO_DATA_BUF_CNT>>1);
		}
        
        
    }
}
#endif
