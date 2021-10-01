#include <util/delay.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "TWIlib.h"

#define ADDR 1
#define SAMPLE 1
#define GET 2
#define SET 3
#define APPLY 4

uint8_t toSet = 0x00;
uint8_t sampled = 0;


void SETHandler(volatile uint8_t* data,uint8_t dataLen){
     if(dataLen!=2) return;
     uint8_t payload = data[1];
     toSet = payload;
}

void APPLYHandler(volatile uint8_t* data,uint8_t dataLen){
    if(dataLen!=1) return;
    PORTB = toSet;
}

void SAMPLEHandler(volatile uint8_t* data,uint8_t dataLen){
  if(dataLen!=1) return;
  sampled = PINA;
}

void GETHandler(volatile uint8_t* data,uint8_t dataLen){
  if(dataLen!=1) return;
  uint8_t TXData[1] = {sampled};
  TWISlaveSendData(TXData,1);
}

int main(void){

  DDRB |= 0xFF;  //Port B used as output
  DDRA &= 0x00;  //Port A used as input
  PORTB =  0;    //Set Port B to Low

  cli();
  installHandler(SETHandler,SET);
  installHandler(APPLYHandler,APPLY);
  installHandler(SAMPLEHandler,SAMPLE);
  installHandler(GETHandler,GET);
  TWIInitSlave(ADDR);
  sei();

  while(1){sleep_mode();}

}


