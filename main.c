#include "main.h"
#include "SD.h"
#include "ADC.h"

void DelayMiliSec(uint16_t MiliSec)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->SMCR &= ~TIM_SMCR_SMS; 
	TIM3->PSC=72;
	TIM3->ARR=1000;
	TIM3->CR1 |= 	TIM_CR1_CEN;
	uint16_t Count=0;
	while (Count<MiliSec)
	{
		if (TIM3->CNT==1000) {Count++; TIM3->CNT=0;}
	}
	TIM3->CR1 &= ~TIM_CR1_CEN;
}

uint16_t res=0;

int main()
{

	SystemInit();
	__disable_irq();

	RCC->CR |= RCC_CR_HSEBYP;
	RCC->CR |= RCC_CR_HSEON;
	
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPBEN  | RCC_APB2ENR_IOPAEN  | RCC_APB2ENR_AFIOEN;	// Разрешить тактирование портов A B 
	AFIO->MAPR = 0x02000000;
	
	GPIOA->CRL &= ~GPIO_CRL_CNF3;  GPIOA->CRL |= GPIO_CRL_MODE3;
	GPIOA->CRH &= ~GPIO_CRH_CNF15; GPIOA->CRH |= GPIO_CRH_MODE15; //настраиваем как двухтактный выход, с максимальной частотой 50MHz
	
	GPIOC->CRH &= ~GPIO_CRH_CNF13; GPIOC->CRH |= GPIO_CRH_MODE13;
	
	//------------------ ПИНЫ SPI -----------------
	GPIOA->CRL &= ~GPIO_CRL_CNF4;  GPIOA->CRL |= GPIO_CRL_MODE4;
	GPIOA->CRL &= ~GPIO_CRL_CNF7;  GPIOA->CRL |= GPIO_CRL_MODE7;
	GPIOA->CRL &= ~GPIO_CRL_CNF5;  GPIOA->CRL |= GPIO_CRL_MODE5;
	//---------------------------------------------
		
	/*GPIOA->LCKR = 0xfffff; GPIOA->LCKR = 0; GPIOA->LCKR = 0xfffff;
	uint32_t temp = GPIOA->LCKR;
	temp = GPIOA->LCKR;*/
	
	__enable_irq();
	
	res = SD_init();
	ReadBootSector();
	
	InitADC();
	NewWavFile();
	
	uint8_t led=15;
	GPIOC->BRR |= 1<<13; GPIOA->BRR |= 1<<15; GPIOA->BRR |= 1<<3;
 
while (1)
  {
		
		if ((RawFile.Cluster % Cluster_BUFF_SIZE > Cluster_BUFF_SIZE/2)&&(FatAdress1<FatAdress2))
		{
			GPIOC->BSRR |= 1<<13;
			
			WriteSector(FatAdress1,   (uint8_t*)  Clusters);
			DelayMiliSec(200);
			WriteSector(FatAdress1+(LongFat/2) ,  (uint8_t*)  Clusters);
			DelayMiliSec(200);
			FatAdress1 = FatAdress2+1;
			ReadSector(FatAdress1, (uint8_t*)  Clusters);
			
			PrintStrAndNumber("Read FAT! FatAdress1:", 21, FatAdress1);
			GPIOC->BRR |= 1<<13;			
		}			
		
		if ((RawFile.Cluster % Cluster_BUFF_SIZE < Cluster_BUFF_SIZE/2)&&(FatAdress1>FatAdress2))
		{
			GPIOC->BSRR |= 1<<13;
			WriteSector(FatAdress2,               (uint8_t*)  &Clusters[Cluster_BUFF_SIZE/2]);
			DelayMiliSec(200);
			WriteSector(FatAdress2+(LongFat/2) ,  (uint8_t*)  &Clusters[Cluster_BUFF_SIZE/2]);
			DelayMiliSec(200);
			FatAdress2 = FatAdress1+1;
			ReadSector(FatAdress2, (uint8_t*)  &Clusters[Cluster_BUFF_SIZE/2]);
			
			PrintStrAndNumber("Read FAT! FatAdress2:", 21, FatAdress2); 
			GPIOC->BRR |= 1<<13;
		}
		
		PrintStrAndNumber("cluster File:", 13, RawFile.Cluster);  
		PrintStrAndNumber("Size File:", 10, RawFile.SizeF);
		PrintStr0("\r ------------------------ \r\0");
		
		
		//if (RawFile.SizeF>=31457280)
		if (RawFile.Cluster - RawFile.BeginFile >= 768)
		{
			GPIOA->BRR |= 1<<led; 
			if (led==3) led=15; else led=3;
			
			SaveFile();
			NewWavFile();
		}
			GPIOA->BSRR |= 1<<led;
			DelayMiliSec(300);
			GPIOA->BRR |= 1<<led;
			DelayMiliSec(100);		

	}	
	
}	