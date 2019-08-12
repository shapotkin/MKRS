/*
��. �������� � ./src/dp.c
 */

#ifndef DP_H
#define DP_H

// ��������� wPi, � �� ������ ��������� �� �����
// ��. gpio readall

#define	ADC_A0		19	
#define	ADC_A1		24	

#define	ASA0		3	
#define ASA1		2	
#define ASA2		7	
#define ASA3		1

#define	STROBE		6
#define ATT_DATA	12
#define ATT_CLK		14
#define LE		10
#define LE_AMP		13

#define SPI1_CS		20 
#define SPI1_CLK	21
#define SPI1_MISO	23
#define SPI1_MOSI	22

void InitBoard();	// инициализация GPIO
void pStrobe(int, int); // положительный строб
void nStrobe(int, int); // отрицательный строб
int attSet(int, float); // установка по адресу значения аттенюатора
int attAmpSet(float);   // установка аттенюатора передатчика МКРС
int GetCPUTemp();	// чтение температуры процессора

void read_ADC(int);	// чтение значений всех АЦП

#endif /* DP_H */
