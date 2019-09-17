/* Includes ------------------------------------------------------------------*/
#include "include.h"
#include "nx1_gpio.h"
#if _ACTION_MODULE
#include "IoTab.h"
#include "nx1_def.h"
#include "nx1_timer.h"
#endif
#include "debug.h"
/* Defines -------------------------------------------------------------------*/

#define BUFFER_NONE         0x00
#define BUFFER_START        0x01
#define BUFFER_MIDDLE       0x02
//

#define PIN_MASK    0x0F
#define PORT_MASK   0xF0
#define VIO_END_FLAG        0x8000
#define VIO_MARK_FLAG       0x4000
#define VIO_END_OUT			0x2000

/* Static Variables ----------------------------------------------------------*/



/* Extern Tables ----------------------------------------------------------*/
#if _ACTION_MODULE
extern volatile ACTION_IO_CTRL  Action_io_ctrl[_VIO_CH_NUM];
extern volatile S8  g_semaphore;
#endif
//------------------------------------------------------------------//
// Check if any vio control event
// Parameter: 
//          NONE
// return value:
//          NONE
//------------------------------------------------------------------//      
void ChkVioCtl(void)
{
#if _ACTION_MODULE   
    U8 i,/*j,*/port,pin;
    GPIO_TypeDef *p_port;
    U8 val;
    
 	
    for(i=0;i<_VIO_CH_NUM;i++)
	{
		
		if(Action_io_ctrl[i].state==STATUS_PLAYING)
		{
            port=Action_io_ctrl[i].port&PORT_MASK;
    		pin=Action_io_ctrl[i].port&PIN_MASK;
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
            
            while(1)
            {
                // mark , end
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_MARK_FLAG)
                {
                    // TODO
                    // Mark callback
					CB_ActionMark(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt>>8);//get Mark id.
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt &= (~VIO_MARK_FLAG);
					Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt  &= (~0xFF00);//clear Mark id.
                }
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_END_FLAG)
                {
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt &= (~VIO_END_FLAG); //END is dummy frame.   
                    //GPIO_Write(p_port,pin,!Action_io_ctrl[i].busy_level);
					if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_END_OUT)
					{
						GPIO_Write(p_port,pin,Action_io_ctrl[i].busy_level);
					}
					else
					{
						GPIO_Write(p_port,pin,!Action_io_ctrl[i].busy_level);
					}
                    Action_io_ctrl[i].state = STATUS_STOP;
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
                    
                    //dprintf("test_cnt %d idx %d\r\n",test_cnt,Action_io_ctrl[i].rd_index);
                    return;
                }
                
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt)
                {
                    val = Action_io_ctrl[i].busy_level;
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt--;
                    GPIO_Write(p_port,pin,val);
                    //test_cnt++;
                    //dprintf("high cnt %d\r\n",Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt);
                    break;
                }
                else if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt)
                {
                    val = Action_io_ctrl[i].busy_level ^ 0x01;
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt--;
                    GPIO_Write(p_port,pin,val);
                    //test_cnt++;
                    //dprintf("low cnt %d\r\n",Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt);
                    break;
                }

                Action_io_ctrl[i].rd_index++;
                //dprintf("%d rd_idx %d\r\n",i,Action_io_ctrl[i].rd_index);
                if(Action_io_ctrl[i].rd_index == VIO_DATA_BUF_CNT)
                {
                    Action_io_ctrl[i].rd_index=0;
                    Action_io_ctrl[i].wr_req = BUFFER_MIDDLE;
                }
                else if(Action_io_ctrl[i].rd_index == (VIO_DATA_BUF_CNT>>1)) 
                {
                    Action_io_ctrl[i].wr_req = BUFFER_START;
                }
            }
		}
		
		else if(Action_io_ctrl[i].state==STATUS_PAUSE)
		{
            U8 index;
            
            port=Action_io_ctrl[i].port&PORT_MASK;
    		pin=Action_io_ctrl[i].port&PIN_MASK;
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
            
            while(1)
            {
                // mark , end
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_MARK_FLAG)
                {
                    // TODO
                    // Mark callback
                    CB_ActionMark(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt>>8);//get Mark id.
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt &= (~VIO_MARK_FLAG);
					Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt  &= (~0xFF00);//clear Mark id.
                }
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_END_FLAG)
                {
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt &= (~VIO_END_FLAG);    
                    //GPIO_Write(p_port,pin,!Action_io_ctrl[i].busy_level);
					if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt & VIO_END_OUT)
					{
						GPIO_Write(p_port,pin,Action_io_ctrl[i].busy_level);
					}
					else
					{
						GPIO_Write(p_port,pin,!Action_io_ctrl[i].busy_level);
					}
                    Action_io_ctrl[i].state = STATUS_STOP;
                    return;
                }
                
                if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt)
                {
                    val = Action_io_ctrl[i].busy_level;
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt--;
                    GPIO_Write(p_port,pin,val);
                    break;
                }
                else if(Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt)
                {
                    val = Action_io_ctrl[i].busy_level ^ 0x01;
                    Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt--;
                    GPIO_Write(p_port,pin,val);
                    break;
                }

                // reduce code, use next frame to do cur frame
                
                index = Action_io_ctrl[i].rd_index+1;
                if(index == VIO_DATA_BUF_CNT)
                {
                    index=0;           
                }
                Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].high_duty_cnt = Action_io_ctrl[i].frame_cnt[index].high_duty_cnt;
                Action_io_ctrl[i].frame_cnt[Action_io_ctrl[i].rd_index].low_duty_cnt  = Action_io_ctrl[i].frame_cnt[index].low_duty_cnt;           
            }
		}
			
	}    
#endif    
}    
