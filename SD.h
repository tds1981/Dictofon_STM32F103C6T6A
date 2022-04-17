#ifndef SD_H_
#define SD_H_
//#include "stm32f1xx_hal.h"
#endif /* SD_H_ */

#include "main.h"

#define SS_set 	  	GPIOA->BSRR |= 1<<4      
#define SS_reset  	GPIOA->BRR |= 1<<4 

#define MOSI_set 	  GPIOA->BSRR |= 1<<7 
#define MOSI_reset 	GPIOA->BRR |= 1<<7 

#define SCK_set 		GPIOA->BSRR |= 1<<5
#define SCK_reset   GPIOA->BRR |= 1<<5
 
#define MISO_status  (GPIOA->IDR&0x40)>>6


struct RootRecord
{
	char NameFile[11];
	uint8_t Attribut;
	uint8_t Rezerv;
	uint8_t Sec; //Сотые доли секунды создания файла
	uint16_t Time;
	uint16_t Data;
	uint16_t DataUse;
	uint16_t FirstClusterH;
	uint16_t TimeUseLast;
	uint16_t DataUseLast;
	uint16_t FirstClusterL;
	uint32_t SiseInByte;
};

struct DescriptorFile
{
	uint16_t NumberRecord;
	uint32_t BeginFile; //First cluster
	uint32_t SizeF;
	uint32_t Cluster;
	uint16_t Sector;
	uint32_t AdressSector;
	char NameFile[12];
	uint32_t AdresSectorRootRec;
	uint16_t OffsetSectorRootRec;
};

//extern uint32_t StartPaticion;
extern uint8_t  SectorPerCluster;
//extern uint16_t RootEntries ;
extern uint32_t RootStart;
extern uint32_t FatStart;
extern uint32_t LongFat;
extern uint16_t TypeAdresBlok;


uint8_t Send_SPI(uint8_t dt);
uint8_t SD_cmd(uint8_t b0, uint32_t i, uint8_t b5);
uint16_t SD_init();
uint32_t ReadMBR();
struct DescriptorFile FindFileInRoot(uint16_t REntries, char* NameFile, int8_t LenStr);
uint32_t ReadBootSector();
uint8_t CommandReadSector(uint32_t FirstByteOffset);
//uint8_t WhaitDatatoken();
uint32_t ClasterToSector(uint32_t Claster);
uint32_t NextClusterAdress(uint32_t LastCl);
void ClusterNewValue(uint32_t Claster, uint32_t Value);

uint8_t ReadSector(uint32_t AdresSector, uint8_t *buf);
uint8_t WriteStr(uint32_t AdresBlock, uint16_t Pos ,uint8_t* Str, int16_t LenthStr);
uint8_t WriteSector(uint32_t AdresBlock, uint8_t* Block);
void WriteInFile(struct DescriptorFile *DF, uint8_t* Block);

int16_t pos(uint8_t *MainStr, int16_t LenM, uint8_t *Sub, int16_t LenS);
struct DescriptorFile CreateFile(char NameF[11]);
void NextClusterForFile(struct DescriptorFile *DF);
void IntSaveSector(uint32_t SectorAdres, uint16_t Offset, uint32_t Num);
void SPI_config();