#include "USB.h"

#define DIG_BASE  10 
unsigned char SYMBOLS[DIG_BASE] = {'0','1','2','3','4','5','6','7','8','9'};

void PrintStrAndNumber(char *str, uint8_t len, uint32_t NUM)
{
	char *resStr=(char*)malloc((len+12) * sizeof(char));
	//char resStr[25];
	int m;

	for (uint8_t k=0; k<len; k++) {resStr[k]=str[k];}
	resStr[len]=' ';
	
	for(int8_t i=10; i>=1; --i)
	{
		m = NUM % DIG_BASE; 
		NUM /= DIG_BASE;  
		resStr[len+i]=SYMBOLS[m]; 
	}
	
	resStr[len+11]='\r';
	//CDC_Transmit_FS(resStr, len+12);
	free(resStr);
	DelayMiliSec(50);
}
//-------------------------------------------------------------------------
void PrintStr0(char *str)
{
	uint8_t lenStr;
	for (lenStr=0; str[lenStr]; lenStr++);
	//CDC_Transmit_FS(str, lenStr);
	DelayMiliSec(150);
}
//----------------------------------------------------------------------
#define MAX_SIZE 7 
void IntToStr(uint32_t NUM, char* str)
{
	int i, m;

	for(i=MAX_SIZE-1; i>=0; --i)
	{
		m = NUM % DIG_BASE; 
		NUM /= DIG_BASE;  
		str[i]=SYMBOLS[m]; 
	}
}

