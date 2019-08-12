/*
см. описание в  ./dp.c
 */

#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "dp.h"

extern  double ADC[16];		// значение на входах АЦП

int main (int argc, char* argv[])
{
  int i,y;
  int 		channel;	// номер канала/адрес аттенюатора в системе
  double 	attenuation;    // желаемое затухание в дБ

  if (argc < 2) {
    printf("Usage:\n");
    printf("\tdemo test\n");
    printf("\t\t ATT LEDs on/off\n\n");
    printf("\tdemo att N V\n");
    printf("\t\t set attenuator of channel N to value V dB\n");
    printf("\t\t N - [0~15]. Select the same blue DIP switch on a board.\n");
    printf("\t\t V - [0~31.5]. Attenuation range in 0.5 dB steps\n\n");
    printf("\tdemo attamp V\n");
    printf("\t\t set attenuator of amplifier to value V dB\n");
    printf("\t\t V - [0~31.5]. Attenuation range in 0.5 dB steps\n\n");
    printf("\tdemo adc\n");
    printf("\t\t read ADC's values. Must be ~0.3V for 1-channel ADC\n");
    printf("\t\t Must be ~1.59V, ~1.13V, ~0.67V, ~0.21V  for 4-channels ADC\n");

    return 0;
  }

  InitBoard();

  if (argc ==2 && !strcmp(argv[1], "test")){
    //running lights
    printf("ATT LEDs blinking...\n");
    digitalWrite(ATT_DATA,HIGH);
    for (i=0;i<8;i++)
    pStrobe(ATT_CLK,30000);
    delay(500);
    digitalWrite(ATT_DATA,LOW);
    for (i=0;i<8;i++)
    pStrobe(ATT_CLK,30000);
  }

  if (argc == 4 && !strcmp(argv[1], "att")){
    // установка затухания аттенюаторов
    channel = atoi(argv[2]);
    attenuation = atof(argv[3]);

    if (channel<0 || channel >15){
	printf("Channel number must be in 0..15 range\n");
	return -1;
    }
    if (attenuation<0 || attenuation >31.5){
	printf("Attenuation must be in 0..31.5 range\n");
	return -1;
    }

    printf("Selected channel: %d\n",channel);
    printf("Attenuation: %.1f\n",(float)attenuation);
    printf("Real attenuation: %.1f\n",attSet(channel,(float)attenuation)*0.5);    
  }

  if (argc == 3 && !strcmp(argv[1], "attamp")){
    // Установка затухания аттенюатора в передатчике МКРС
    attenuation = atof(argv[2]);

    if (attenuation<0 || attenuation >31.5){
        printf("Attenuation must be in 0..31.5 range\n");
	return -1;
    }

    printf("Attenuation: %.1f\n",(float)attenuation);
    printf("Real attenuation: %.1f\n",attAmpSet((float)attenuation)*0.5);    
  }

  if (argc == 2 && !strcmp(argv[1], "adc")){
    printf("\033[2J");
    printf("\033[0;0H");
    printf("Температура ЦПУ:\t\tCtrl+C завершение\n");
    printf("---------------------------------------------------------------------------------------\n");

    printf("Адрес АЦП\t00\t01\t10\n");
    printf("Входы АЦП\n");
    for(i=0;i<16;i++)
	printf("\t%d\n",i);

    for(;;){
	for(y=0;y<3;y++){
	    printf("\033[0;18H%d C",(GetCPUTemp()/1000));
	    for(i=0;i<16;i++)
		ADC[i]=0;
	    read_ADC(y);
    	    for (i=0;i<16;i++){
    		printf("\033[%d;%dH%.2f",i+5,17+y*8,ADC[i]);
	    }
	    usleep(100000); //  not less 100 msec
	}
	printf("\033[22;0H");
    }
  }


  return 0 ;
}
