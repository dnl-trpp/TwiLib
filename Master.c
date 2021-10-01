#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "./avr_common/uart.h"
#include "TWIlib.h"

#define SAMPLE 1
#define GET 2
#define SET 3
#define APPLY 4

int main(void){
  cli();
  printf_init();
  TWIInitMaster();
  sei();
  

  uint8_t TXData[3];
  uint8_t addr;
    
  while(1){

    switch(usart_getchar()){

      case 's':
        addr=usart_getchar();
        TXData[0] = (addr<<1);  //Address + W bit (0)
        TXData[1] =  SET;                  // Sample, get ,set or apply
        TXData[2] = usart_getchar();       //Payload (Pins to Set)
        TWIMasterTransmitData(TXData,3,0);
        while(!isTWIReady()) {_delay_us(1);}
        if(getTWIErrorCode() == TWI_SUCCESS){
            usart_putchar(0xFF);
        }
        else{
            usart_putchar(getTWIErrorCode());
        }
        break;
      

      case 'm':
        TXData[0] =   0x00;                  //General call + W bit (0)
        TXData[1] =  SAMPLE;                 // Sample, get ,set or apply
        TWIMasterTransmitData(TXData,2,0);   
        while(!isTWIReady()) {_delay_us(1);}
        if(getTWIErrorCode() == TWI_SUCCESS){
            usart_putchar(0xFF);
        }
        else{
            usart_putchar(getTWIErrorCode());
        }
        break;


      case 'g':
        addr=usart_getchar();
        TXData[0] = (addr<<1);            //Address + W bit (0)
        TXData[1] =  GET;                 // Sample, get ,set or apply
        TWIMasterTransmitData(TXData,2,0);
        while(!isTWIReady()) {_delay_us(1);}
        if(getTWIErrorCode() == TWI_SUCCESS){
          _delay_ms(100);
          TWIMasterReadData(addr,1,0);
          while(!isTWIReady()) { _delay_us(1);}
          if(getTWIErrorCode() == TWI_SUCCESS){    
              usart_putchar(0xFF);
              usart_putchar(TWIReceiveBuffer[0]);
          }
          else{
              usart_putchar(getTWIErrorCode());
          }
        }else{
           usart_putchar(getTWIErrorCode());
        }
        
        break;


      case 'a':
        TXData[0] =   0x00;                  //General call + W bit (0)
        TXData[1] =  APPLY;                 // Sample, get ,set or apply
        TWIMasterTransmitData(TXData,2,0);
        while(!isTWIReady()) {_delay_us(1);}
        if(getTWIErrorCode() == TWI_SUCCESS){
            usart_putchar(0xFF);
        }
        else{
            usart_putchar(getTWIErrorCode());
        }
        break;
      
      default:
            usart_putchar(0xFF);
    }
  }
}


