/* Includes ------------------------------------------------------------------*/

#include "include.h"
#include "nx1_spi.h"

#if _AUDIO_MODULE==1

//------------------------------------------------------------------//
// Read data from storage (called by lib)
// Parameter: 
//          *cur_addr: current address
//          *buf     : data buffer
//          *rec_addr: recorder address
//          size     : data size to read(bytes)
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_ReadRecData(U32 *cur_addr, U8 *buf,RecordAddress_T *rec_addr,U16 size)
{
    if( (rec_addr->startaddr_2==0) || ((*cur_addr+size) < rec_addr->endaddr_1) )
    {
        SPI_BurstRead(buf,*cur_addr,size);
        *cur_addr+=size;
    }
    else
    {
        U16 len1,len2;
        
        len1 = rec_addr->endaddr_1 - *cur_addr;
        if(len1)    SPI_BurstRead(buf,*cur_addr,len1);
        *cur_addr = rec_addr->startaddr_2;
        rec_addr->startaddr_2=0;
        len2 = size - len1;
        if(len2)    SPI_BurstRead((buf+len1),*cur_addr,len2);
        *cur_addr += len2;
        //dprintf("len1 %d len2 %d size %d\r\n",len1,len2,size);
    }
    
}  
//------------------------------------------------------------------//
// Read data from storage (called by lib)
// Parameter: 
//          mode     : storage mode
//          *cur_addr: current address
//          *buf     : data buffer
//          size     : data size to read(bytes)
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_ReadData(U8 mode, U32 *cur_addr, U8 *buf,U16 size){
    U16 i;
    U8 *psrc;
    
    if(mode==SPI_MODE){
        SPI_BurstRead(buf,*cur_addr,size);
    }else if(mode==OTP_MODE){
        psrc=(U8 *)*cur_addr;
        for(i=0;i<size;i++){
            *buf++=  *psrc++;  
        }        
    }
#if _SPI1_MODULE && _SPI1_USE_FLASH
	else if(mode==SPI1_MODE){
		SPI1_BurstRead(buf,*cur_addr,size);
	}		
#endif
    *cur_addr+=size;
}    

                
//------------------------------------------------------------------//
// Write data to storage (called by lib)
// Parameter: 
//          *cur_addr: current address
//          *buf     : data buffer
//          *rec_addr: recorder address
//          size     : data size to read(bytes)
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_WriteRecData(U32 *cur_addr,U8 *buf,RecordAddress_T *rec_addr,U16 size)
{
    if( ((*cur_addr)+size) >= rec_addr->endaddr_1)
    {
        U32 len1,len2;

        len1 = rec_addr->endaddr_1 - *cur_addr;
        if(len1)
        {
            SPI_BurstWrite(buf,*cur_addr,len1);
        }
        len2 = size - len1;
        *cur_addr = rec_addr->startaddr_1;
        if(len2)
        {
            SPI_BurstWrite(buf+len1,*cur_addr,len2);
			*cur_addr+=len2;
        }
        
    }
    else
    {
        SPI_BurstWrite(buf,*cur_addr,size);
        *cur_addr+=size;
    }
}           
//------------------------------------------------------------------//
// Write data to storage (called by lib)
// Parameter: 
//          mode     : storage mode
//          *cur_addr: current address
//          *buf     : data buffer
//          size     : data size to read(bytes)
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_WriteData(U8 mode, U32 *cur_addr,U8 *buf,U16 size){
    
    if(mode==SPI_MODE){
  
        SPI_BurstWrite(buf,*cur_addr,size);

    }        
    *cur_addr+=size;
}    
//------------------------------------------------------------------//
// Write header to storage (called by lib)
// Parameter: 
//          mode     : storage mode
//          *cur_addr: current address
//          *buf     : data buffer
//          size     : data size to read(bytes)
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_WriteHeader(U8 mode, U32 *cur_addr,U8 *buf,U16 size){
    
    if(mode==SPI_MODE){
        SPI_BurstWrite(buf,*cur_addr,size); 
    }     
}    
//------------------------------------------------------------------//
// Get file addr from index
// Parameter: 
//          index: file index or absolute address
//          mode:OTP_MODE,SPI_MODE,SD_MODE,SPI1_MODE 
//          rw:1:write, 0:read
// return value:
//          address
//------------------------------------------------------------------// 
static U32 GetStartAddr(U32 index,U8 mode,U8 rw){
    U8 tempbuf[SPI_TAB_INFO_ENTRY_SIZE];
    U32 addr=0;	//jhua
    //U8 sts;
    
    if(mode==SPI_MODE){
        if(index&ADDRESS_MODE){
            addr=index&(~ADDRESS_MODE);
        }else{    
            addr=SpiOffset();
            
            SPI_BurstRead(tempbuf,addr+SPI_BASE_INFO_SIZE+(SPI_TAB_INFO_ENTRY_SIZE*index),SPI_TAB_INFO_ENTRY_SIZE);
            addr=addr+(tempbuf[0]|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24));
            
            addr&=(~SPI_DATA_TYPE_MASK);	
            //sts=tempbuf[3]>>4;
        }    
    }else if(mode==OTP_MODE){
        addr=PlayList[index];
    }
#if _SPI1_MODULE && _SPI1_USE_FLASH
	else if(mode==SPI1_MODE){
        if(index&ADDRESS_MODE){
            addr=index&(~ADDRESS_MODE);
        }else{    
            addr=Spi1_Offset();
            
            SPI1_BurstRead(tempbuf,addr+SPI_BASE_INFO_SIZE+(SPI_TAB_INFO_ENTRY_SIZE*index),SPI_TAB_INFO_ENTRY_SIZE);
            addr=addr+(tempbuf[0]|(tempbuf[1]<<8)|(tempbuf[2]<<16)|(tempbuf[3]<<24));
  
            addr&=(~SPI_DATA_TYPE_MASK);	
            //sts=tempbuf[3]>>4;
        } 		
	}
#endif    
    return addr;
}  
//------------------------------------------------------------------//
// Set start address of file(called by lib)
// Parameter: 
//          addr       : file index or absolute address
//          mode       : OTP_MODE,SPI_MODE,SD_MODE,SPI1_MODE
//          rw:1       : write, 0:read
//          *start_addr: current address
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_SetStartAddr(U32 addr,U8 mode,U8 rw,U32 *start_addr){
    *start_addr = GetStartAddr(addr,mode,rw); 
}     
//------------------------------------------------------------------//
// Get start address
// Parameter: 
//          *index:midi file index
//          *addr:Timbre start address
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_Midi_GetStartAddr(U32 *index,U32 *addr){
    U8 temp_buf[SPI_TAB_INFO_ENTRY_SIZE];
    U32 offset;

	offset=SpiOffset();
	
	if((*index)&ADDRESS_MODE){
		(*index)=(*index)&(~ADDRESS_MODE);
	}else{
		SPI_BurstRead(temp_buf,offset+SPI_BASE_INFO_SIZE+(SPI_TAB_INFO_ENTRY_SIZE*(*index)),SPI_TAB_INFO_ENTRY_SIZE);   
		*index=offset+(temp_buf[0]|(temp_buf[1]<<8)|(temp_buf[2]<<16)|(temp_buf[3]<<24));
		(*index)&=(~SPI_DATA_TYPE_MASK);
	}	
	
	SPI_BurstRead(temp_buf,offset+SPI_TIMBRE_ADDR_OFFSET,4);	
	*addr=offset+(temp_buf[0]|(temp_buf[1]<<8)|(temp_buf[2]<<16)|(temp_buf[3]<<24));
	
}   
//------------------------------------------------------------------//
// Seek from start address(called by lib)
// Parameter: 
//          offset: seek offset
// return value:
//          NONE
//------------------------------------------------------------------// 
void CB_Seek(U32 offset){  
   
}

#if _AUDIO_SENTENCE   
//------------------------------------------------------------------//
// SentenceOrderPlay CallBack function
// Parameter: 
//          ch: audio channel or action channel
//			markNum: mark number 1~255 that setup by SPI_Encoder
//			sentenceCBorder: sequence of the CallBack on this sentence order
// return value:
//          NONE
//------------------------------------------------------------------//  
void CB_Sentence(U8 ch, U8 markNum, U8 sentenceCBorder){
	
	SentenceCB[ch] = 0;
	
}
#endif
#endif
