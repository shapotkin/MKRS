/*
относится к плате 201905127 MKRS controller
Контроллер управления NanoPi Neo Core
http://nanopi.io/nanopi-neo-core.html
В тестовых программах используется библиотека WiringNP
https://github.com/friendlyarm/WiringNP
Библиотека установлена и скомпилирована по пути /usr/local/src/WiringNP
Для самостоятельной компиляции данной библиотеки на платформе Armbian
требуется создать файл /usr/local/etc/sys_info с содержимым:

sunxi_platform    : Sun8iw7p1
sunxi_secure      : normal
sunxi_chipid      : 2c21020e786746240000540000000000
sunxi_chiptype    : 00000042
sunxi_batchno     : 1
sunxi_board_id    : 5(0)

В исходном файле библиотеки boardtype_friendlyelec.с в функции
getAllwinnerBoardID() заменить ссылку файла с информацией о ЦПУ на 
/usr/local/etc/etc/sys_info
и выполнить компиляцию библиотеки

проверка: gpio readall

должна быть выведена таблица с настройкой выводов платы NanoPi Neo Core
Назачение выводов платы NanoPi Neo Core для управления компонентами:
Pin		Сигнал		Назначение
--------------------------------------------------------------------
Сигналы адресации аттенюаторов оптических передатчиков/усилителей	
15/GPIOA3	ASA0	Младший (0) бит адреса выбора аттенюатора
13/GPIOA2	ASA1	(1) бит адреса выбора аттенюатора
11/GPIOA0	ASA2	(2) бит адреса выбора аттенюатора
12/GPIOA6	ASA3	Старший (3) бит адреса выбора аттенюатора
22/GPIOA1	STROBE	фиксация (строб) адреса выбора аттенюатора
        теперь, при активации сигнала LE на выбранном
        аттенюаторе оптического передатчика БОППа или 
        оптического приемника МКРСа зафиксируется установленный
        сигналами ATT_CLK и ATT_DATA уровень затухания
        Занесение нового адреса происходит по фронту
        сигнала STROBE вне зависимости от сигнала LE
24/GPIOC3	LE	Сигнал Latch Enable для выбранного аттенюатора
        оптических передатчиков БОППа или оптических
        приемников МКРСа
        Если LE = 1, то все выходы мультиплексора =0, 
        светодиод MS# погашен
Алгоритм формирования сигнала Latch Enable для аттенюаторов передатчиков БОППа 
и приемников МКРСа:
Исходное состояние: STROBE=0 и LE=1 
Линиями ASA0..ASA3 выставляем двоичный адрес, соответствующий включенному DIP переключателю
Импульсом сигнала STROBE (>125 нс) заносим адрес в муьтиплексор
После формируем отрицательный импульс LE, который сформирует положительный импульс
Latch Enable для выбранного аттенюатора
21/GPIOC1	LE_AMP	Сигнал Latch Enable для аттенюатора передатчика МКРСа
Аттенюатор включен в режим последовательного управления значением затухания
23/GPIOC2	ATT_CLK	Тактирование для занесения величины затухания 
        в аттенюаторы. Частота следования до 10 МГц (длительность
        импульсов >40 нс. Тактирование происходит по фронту импульса
        к этому момнету ATT_DATA должен быть уже установлен
19/GPIOC0	ATT_DATA	Двоичный код уровня затухания.
        Всего необходимо передать 6 бит. Начало передачи со старшего бита
        000000	референсное затухание
        000001	0.5 дБ
        000010  1 дБ
        ...
        111111	31.5 дБ    
13/GPIOA17	ADC_A0	Младший (0) бит адреса выбора АЦП
11/GPIOL11	ADC_A1	Старший (1) бит адреса выбора АЦП
        00 - четырехканальный АЦП ADS7950
        01,10 - 16-ти канальный АЦП ADS7953
SPI1	SPI-шина для управления АЦП. назначение выводов управления см. в ./inc/dp/h
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include "dp.h"


// для формирования наносекундных задержек можно использовать
// функции times, clock и др.
// см. https://habr.com/ru/post/282301/

// формирование импульса высокого уровня на pin 
// длительностью duration микросекунд

void pStrobe(int pin, int duration){
    digitalWrite(pin, HIGH);
    usleep((unsigned int)duration);
    digitalWrite(pin, LOW);
}

// формирование импульса низкого уровня на pin 
// длительностью duration микросекунд

void nStrobe(int pin, int duration){
    digitalWrite(pin, LOW);
    usleep((unsigned int)duration);
    digitalWrite(pin, HIGH);
}

double	ADC[16];	// значения напряжений на входах АЦП. 

// чтение результатов по всем  каналам АЦП в "ручном" режиме
// см. п.8.4.4 по ADS7950

void read_ADC(int address){

    // address - адреса выбора многоканального АЦП

    int chan,chan_total,i,data_out, data_in;
    // data_out - команда, посылаемая в АЦП, в которой сразу закодирован
    //		  ручной режим и адрес канала АЦП. см. п.8.4.4 по ADS7950 Table 1

    // data_in  - ответ от АЦП. первые 4 бита будут содержать адрес канала,
    //		  которому относится результат измерений

    // D15..D12: 0001  	ручной режим. измеряемый канал передается в текущем фрейме
    //			результат будет через фрейм позже
    // D11:	 0	биты D6..D0 повторяютя из предыдущего фрейма
    //	 	 1	биты D6..D0 задаются новыми
    // D10..D7:	 0000	адрес следующего канала, который будет выбран в следующем фрейме
    //			D10 - MSB, D7 - LSB
    // D6:	 0	Диапазон измеряемых напряжений 0..2,5В
    //		 1	Диапазон измеряемых напряжений 0..5В
    // D5:	 0	Нормальный режим работы
    //		 1	АЦП уходит в спячку после 16 CLK импульсов
    // D4:	 0	На выходе АЦП SDO в DO15..DO12 будет номер канала,
    //			а на выходах DO11..DO0 - результат измерений
    //		 1	на выходе АЦП SDO в DO15..DO12 - состояние GPIO3..0
    //			а на выходах DO11..DO0 - результат измерений текущего канала
    // D3..D0		данные для установки GPIO. Не используется

    // значение в ADC[3] для АЦП 00 и ADC[15] для АЦП 01 и АЦП 10 - значения температура/напряжение
    // от встроенного в источник опорного напряжения преобразователя. см. документацию к REF5025
    // можно использовать как контрольную точку для самодиагностики и контроля температуры в модулях МКРС

    // значение ADC[14] для АЦП 01 и АЦП 10 - напряжение от фиксированного делителя напряжения
    // должно быть примерно 0.17 Вольта. Можно использовать как контрольную точку для самодиагностики

    if ((address < 0) || (address > 2)){
	// в контроллере МКРС только 3 АЦП
	printf("error\n");
        return;
    }
    if (address == 0)
	chan_total = 4; // у АЦП 00 только 4 канала
    else
	chan_total = 16; // у АЦП 01 и 10 все 16 каналов
    
    // выбрали адрес АЦП
    digitalWrite(ADC_A0,address&1);
    digitalWrite(ADC_A1,(address&2)>>1);

    digitalWrite(SPI1_CLK,LOW);
    // выбрали АЦП
    digitalWrite(SPI1_CS,LOW);

    for (chan=0;chan<chan_total+2;chan++){
	if (chan>chan_total-1)
	    data_out = 0; // для получения последних двух каналов в АЦП слать ничего не надо
	else
    	    data_out = ((1<<5)+chan)<<7; // см. п.8.4.4 по ADS7950

	data_in = 0;
	for(i=15;i>=0;i--){
    	    digitalWrite(SPI1_MOSI,(data_out>>i)&1);
    	    digitalWrite(SPI1_CLK,HIGH);
	    // usleep(1); // быстродействия АЦП хватает для работы без задержек
            if (chan>1){  // пропустили первые 2 "ответа"
		data_in=(data_in<<1)|digitalRead(SPI1_MISO);
	    }
    	    digitalWrite(SPI1_CLK,LOW);
	    // usleep(1); // быстродействия АЦП хватает для работы без задержек
	}
	if (chan >1){
	    // в первых 4-х битах - номер порта, по которому произведено измерение
	    // в оставшихся 12-ти битах - результат измерения относительно 2.5 Вольта
	    // 12-ти битный АЦП, опорное напряжение 2.5В

	    ADC[(data_in>>12)]	= 2.5*(data_in&4095)/4096; 
	}
	pStrobe(SPI1_CS,1);
    }
    // снимаем выбор АЦП
    digitalWrite(SPI1_CS,HIGH);

    digitalWrite(ADC_A0,HIGH);
    digitalWrite(ADC_A1,HIGH);

}


void InitBoard(){
  wiringPiSetup () ;
  // назначение выходов также устанавливается в файле rc.local
  pinMode(ADC_A0,OUTPUT);
  pinMode(ADC_A1, OUTPUT);
  digitalWrite(ADC_A0,HIGH);
  digitalWrite(ADC_A1, HIGH);

  pinMode(ASA0, OUTPUT);
  pinMode(ASA1, OUTPUT);
  pinMode(ASA2, OUTPUT);
  pinMode(ASA3, OUTPUT);

  pinMode(STROBE, OUTPUT);
  pinMode(ATT_DATA, OUTPUT);
  pinMode(ATT_CLK, OUTPUT);
  pinMode(LE, OUTPUT);
  digitalWrite(LE,HIGH);	// все адресные выходы с мультиплексора выбора аттенюатора в 0
  pinMode(LE_AMP, OUTPUT);

// SPI1 шину эмулируем программно
  pinMode(SPI1_CS, OUTPUT);
  pinMode(SPI1_CLK, OUTPUT);
  pinMode(SPI1_MISO, INPUT);
  pinMode(SPI1_MOSI, OUTPUT);
  digitalWrite(SPI1_CS,HIGH);

  return;
}

int attSet(int ch, float att){
    int i, att_code=0;

    while ((att_code*0.5) < att)
        att_code++;

// установить адрес/номер аттенюатора в системе
    digitalWrite(ASA0,ch&1);
    digitalWrite(ASA1,(ch>>1)&1);
    digitalWrite(ASA2,(ch>>2)&1);
    digitalWrite(ASA3,(ch>>3)&1);
    pStrobe(STROBE,1);// достаточно более 250 нсек

    // передаем код затухания
    // сначала 2 нуля, что бы светодиоды соответствовали
    // заодно "вытеснить" старый мусор из аттенюаторов
    digitalWrite(ATT_DATA,LOW);
    pStrobe(ATT_CLK,1); // достаточно длительности более 3.5 нсек с частотой повторения до 125МГц
    pStrobe(ATT_CLK,1);
    // загружаем код затухания
    for (i=5;i>=0;i--){
	digitalWrite(ATT_DATA,(att_code>>i)&1);
	pStrobe(ATT_CLK,1); // достаточно длительности более 3.5 нсек с частотой повторения до 125МГц
    }

    // софрмировать сигнал Latch Enable в виде короткого положительного импульса
    // отрицательным импульсом LE формируется положительный импульсь Latch Enable для выбранного аттенюатора
    nStrobe(LE,2000000); // 30 нс достаточно

    return att_code;
}

int attAmpSet(float att){
    int i, att_code=0;

    while ((att_code*0.5) < att)
        att_code++;

    // передаем код затухания
    // сначала 2 нуля, что бы светодиоды соответствовали
    // заодно "вытеснить" старый мусор из аттенюаторов

    digitalWrite(ATT_DATA,LOW);
    pStrobe(ATT_CLK,1); // достаточно длительности более 3.5 нсек с частотой повторения до 125МГц
    pStrobe(ATT_CLK,1);
    // загружаем код затухания
    for (i=5;i>=0;i--){
	digitalWrite(ATT_DATA,(att_code>>i)&1);
	pStrobe(ATT_CLK,1); // достаточно длительности более 3.5 нсек с частотой повторения до 125МГц
    }

    // софрмировать сигнал Latch Enable в виде короткого положительного импульса
    pStrobe(LE_AMP,2000000); // 30 нс достаточно

    return att_code;
}
int GetCPUTemp() {
    // чтение температуры процессора
   int FileHandler;
   char FileBuffer[10];
   int CPU_temp;
   FileHandler = open("/sys/devices/virtual/thermal/thermal_zone0/temp", O_RDONLY);
   if(FileHandler < 0) {
      return -1; }
   ssize_t m=read(FileHandler, FileBuffer, sizeof(FileBuffer) - 1);
   m=m;
   sscanf(FileBuffer, "%d", &CPU_temp);
   close(FileHandler);
   return CPU_temp;
}

