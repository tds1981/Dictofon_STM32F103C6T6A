#include "ADC.h"
#include "SD.h"
#include "wav.h"

void InitTimer()
{
		RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; 
		TIM1->SMCR &= ~TIM_SMCR_SMS;  // внутреннее тактирование, шина APB
		TIM1->PSC=9;
		TIM1->ARR=225;
		TIM1->CCR1=TIM1->ARR;
		TIM1->CR2 |= TIM_CR2_MMS_2;
		
	  TIM1->CCMR1 |= TIM_CCMR1_OC1M;                // Ch1: PWM mode2
    TIM1->CCER |= TIM_CCER_CC1E;                  //<! Ch1 out enable
    TIM1->BDTR |= TIM_BDTR_MOE;                    //<! main out enable (!!!)

	//	TIM1->DIER |= TIM_DIER_UIE;
	//	NVIC_EnableIRQ (TIM1_UP_IRQn);
		
		TIM1->CR1 = TIM_CR1_CEN;
}

//----------------------------------------------------------

void InitADC()
{
		RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
		ADC1->CR2 |= ADC_CR2_CAL; 
		while (ADC1->CR2&0x04 != 0){}
		
		ADC1->CR2 &= ~ADC_CR2_ADON;		// выкл ацп
	
		ADC1->CR1 &= ~ADC_CR1_SCAN; 			// запрет режима сканирования
		ADC1->CR2 &= ~ADC_CR2_CONT;				// запрет непрерывного режима
		//ADC1->CR2 |= ADC_CR2_CONT;	 		// разрешение непрерывного режима
		
		ADC1->CR2 |= ADC_CR2_EXTTRIG;  	// разрешение запуска ацп от внешнего события
		//ADC1->CR2 |= ADC_CR2_EXTSEL_0 | ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_2; // SWSTART
		ADC1->CR2 &= ~(ADC_CR2_EXTSEL_0 | ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_2); // TIM1_CC1	
		//ADC1->CR2 |= ADC_CR2_ALIGN;
		ADC1->CR2 |= ADC_CR2_DMA;	
		
		ADC1->SQR1 =0; 
		ADC1->SQR3 =1; // канал А1
		
		ADC1->SMPR1 |= 0x08; //001: 7.5 cycles
			
		ADC1->CR2 |= ADC_CR2_ADON;
			
			
	//	ADC1->CR2 |= ADC_CR2_SWSTART;			
}	

#define ADC1_DR_Address    ((uint32_t)0x40012400+0x4c)
#define DMA_BUFF_SIZE 1024

int16_t ADC_buff[DMA_BUFF_SIZE];
struct DescriptorFile RawFile;
uint32_t FatAdress1;
uint32_t FatAdress2;

//-------------------------------------------------
void InitDMA()
{

		RCC->AHBENR |= RCC_AHBENR_DMA1EN;					//Разрешаем тактирование первого DMA модуля
		DMA1_Channel1->CPAR =  ADC1_DR_Address;		//Указываем адрес периферии - регистр результата преобразования АЦП для регулярных каналов
		DMA1_Channel1->CMAR = (uint32_t)ADC_buff;	//Задаем адрес памяти - базовый адрес массива в RAM
		DMA1_Channel1->CCR &= ~DMA_CCR1_DIR; 		// Указываем направление передачи данных, из периферии в память
		DMA1_Channel1->CNDTR = DMA_BUFF_SIZE; 	// Количество пересылаемых значений
		
		DMA1_Channel1->CCR &= ~DMA_CCR1_PINC; 		//Адрес периферии не инкрементируем после каждой пересылки
		DMA1_Channel1->CCR |= DMA_CCR1_MINC; 		//Адрес памяти инкрементируем после каждой пересылки.
		DMA1_Channel1->CCR |= DMA_CCR1_PSIZE_0; //Размерность данных периферии - 16 бит
		DMA1_Channel1->CCR |= DMA_CCR1_MSIZE_0; //Размерность данных памяти - 16 бит
		DMA1_Channel1->CCR |= DMA_CCR1_PL;		 //Приоритет - очень высокий 
		DMA1_Channel1->CCR |= DMA_CCR1_CIRC; 		//Разрешаем работу DMA в циклическом режиме
		
		DMA1_Channel1->CCR |= DMA_CCR1_HTIE; 	// Разрешение прерывания при достижении половины буфера
		DMA1_Channel1->CCR |= DMA_CCR1_TCIE;
	
		NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	
		DMA1_Channel1->CCR |= DMA_CCR1_EN; 			//Разрешаем работу 1-го канала DMA

}

//----------------------------------------------------

uint32_t Clusters[Cluster_BUFF_SIZE];
//uint16_t ClusterCount;

uint8_t HalfFlag=1;

void StopIRQ()
{
	__disable_irq ();
//	NVIC_DisableIRQ(DMA1_Channel1_IRQn);
	DMA1_Channel1->CCR &= ~DMA_CCR1_EN;
	TIM1->CR1 &= ~TIM_CR1_CEN; 
}

void StartIRQ()
{
		TIM1->CR1 = TIM_CR1_CEN;
		NVIC_EnableIRQ(DMA1_Channel1_IRQn);
		DMA1_Channel1->CCR |= DMA_CCR1_EN;
		__enable_irq ();
}

void SaveFile()
{
		StopIRQ();
		PrintStr0("-------- Save File: ---------- \r\0");
		
		Clusters[RawFile.Cluster % Cluster_BUFF_SIZE]=0x0FFFFFFF;
	  uint32_t NumberSectorFat = RawFile.Cluster/128; // 128- количество fat-полей в одном секторе
		FatAdress1 = FatStart + NumberSectorFat;
		WriteSector(FatAdress1,               (uint8_t*)  &Clusters[(NumberSectorFat%2)*Cluster_BUFF_SIZE/2]);
		WriteSector(FatAdress1+(LongFat/2) ,  (uint8_t*)  &Clusters[(NumberSectorFat%2)*Cluster_BUFF_SIZE/2]);
		
		RawFile.SizeF = (RawFile.Cluster - RawFile.BeginFile)*SectorPerCluster*512;
		IntSaveSector(RawFile.AdresSectorRootRec, 28+RawFile.OffsetSectorRootRec, RawFile.SizeF);
		WAVHEADER header = WavHeader(32000, RawFile.SizeF);
		WriteStr(ClasterToSector(RawFile.BeginFile), 0,  (uint8_t*)&header, sizeof(header));
			
}

void NewWavFile()
{	
			PrintStr0("-------- Create File: ---------- \r\0");
			RawFile = CreateFile("\0WAV");
	
			FatAdress1 = FatStart + RawFile.BeginFile/128; // 128- количество fat-полей в одном секторе
			ReadSector(FatAdress1, (uint8_t*)  Clusters);
			FatAdress2 = FatAdress1 + 1; 
			ReadSector(FatAdress2, (uint8_t*) &Clusters[Cluster_BUFF_SIZE/2]);
		
			PrintStr0("-------- Start record ---------- \r\0");
			
			InitTimer();
			InitDMA();
}	

void NewClaster()
{

}

void DMA1_Channel1_IRQHandler(void)
{
		if ((DMA1->ISR&DMA_ISR_HTIF1)&&HalfFlag) 
		{	
		
	//	CDC_Transmit_FS(ADC_buff, DMA_BUFF_SIZE);
			//if (RawFile.Sector==0)  NextClusterForFile(&RawFile);  //FlagNewCluster=1;
			for (uint16_t i=0; i<DMA_BUFF_SIZE/2; i++) ADC_buff[i]*=6;
			//if (
				WriteSector(RawFile.AdressSector++, (uint8_t*) ADC_buff) ;
			//!= 0) SD_init();
			//if (
				WriteSector(RawFile.AdressSector++, (uint8_t*) &ADC_buff[DMA_BUFF_SIZE/4]); 
			//!= 0) SD_init();
			RawFile.Sector+=2;RawFile.SizeF += 1024;
		
		}
		
	  if (DMA1->ISR&DMA_ISR_TCIF1) // full bufer
		{	
		
		//	CDC_Transmit_FS(&ADC_buff[DMA_BUFF_SIZE/2], DMA_BUFF_SIZE);
			for (uint16_t i=DMA_BUFF_SIZE/2; i<DMA_BUFF_SIZE; i++) ADC_buff[i]*=6;
			//if (
				WriteSector(RawFile.AdressSector++, (uint8_t*) &ADC_buff[DMA_BUFF_SIZE/2]); 
			//!= 0) SD_init();
			//if (
				WriteSector(RawFile.AdressSector++, (uint8_t*) &ADC_buff[3*DMA_BUFF_SIZE/4]);
			// != 0)	SD_init();
			RawFile.Sector+=2; RawFile.SizeF += 1024;
				
		}
		
		if (RawFile.Sector>=SectorPerCluster)
			{
				RawFile.Sector=0;
				//__disable_irq ();
				uint32_t ClearCl=RawFile.Cluster+1;
				//if (ClearCl % Cluster_BUFF_SIZE == 0) { SaveFile();	return;}
				while(Clusters[ClearCl % Cluster_BUFF_SIZE]){ClearCl++;}
				Clusters[RawFile.Cluster % Cluster_BUFF_SIZE]=ClearCl;
				RawFile.Cluster = ClearCl;
				RawFile.AdressSector=ClasterToSector(RawFile.Cluster);
				//__enable_irq ();
			}
		
		DMA1->IFCR |= 0xfffffff;//DMA_IFCR_CGIF1;
}


