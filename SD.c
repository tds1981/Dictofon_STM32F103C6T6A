#include "SD.h"
#include "USB.h"

uint8_t  SectorPerCluster;
uint16_t RootEntries ;
uint32_t RootStart;

uint32_t FatStart;
uint32_t LongFat;
uint16_t TypeAdresBlok=0x200;
uint8_t TypeFat=0; //Fat32

void SPI_config()
{
	RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_AFIOEN ;
	
	GPIOA->CRL &= ~GPIO_CRL_CNF4;  GPIOA->CRL |= GPIO_CRL_MODE4;
	
	GPIOA->CRL &=  ~(GPIO_CRL_CNF7 | GPIO_CRL_CNF5);  
	GPIOA->CRL |=  GPIO_CRL_MODE7  | GPIO_CRL_MODE5;
	GPIOA->CRL |=  GPIO_CRL_CNF5_1 | GPIO_CRL_CNF7_1;// выход с альтернативной функцией push-pull
	
	SPI1->CR1 |=  SPI_CR1_BR_1 | SPI_CR1_BR_0; // CLK/8 			SPI_CR1_BR_1 |
	
	SPI1->CR1|=SPI_CR1_MSTR;	//Режим ведущего
	SPI1->CR1|=SPI_CR1_SSM;		//Программный NSS
	SPI1->CR1|=SPI_CR1_SSI;		//NSS - high
//SPI1->CR2|=SPI_CR2_TXDMAEN;//Разрешить запросы DMA
	
	SPI1->CR1 |= SPI_CR1_SPE; 
}	


uint8_t Send_SPI(uint8_t DATA)
{
	(void) SPI1->DR;
	while(SPI1->SR & SPI_SR_TXE == 0) {}	
  SPI1->DR = DATA;
  while(!(SPI1->SR & SPI_SR_RXNE)) {}
  return SPI1->DR;	
}

/*
uint8_t Send_SPI(uint8_t DATA)
{
		uint8_t res=0;
		for(uint8_t i=1; i<=8; i++)
		{
			if ((DATA&0x80)==0x80) MOSI_set; //  1
				else MOSI_reset; //  0
			SCK_set;
				DATA=DATA<<1;
			 res=(res<<1);
			// for (uint8_t k=0; k<2; k++) {}	
			 if (MISO_status)   res=res|0x01;		
				 else res=res&0xFE;
			SCK_reset;
			//for (uint8_t k=0; k<1; k++) {}	
		}
		return res;
}
*/
uint8_t SD_cmd(uint8_t b0, uint32_t i, uint8_t b5)
{ 
	uint8_t res;
	long int count;
	uint8_t  b1, b2, b3, b4;
	
	b4 = i&0xff;  // long int 
	i=i>>8;
	b3 = i&0xff;
	i=i>>8;
	b2 = i&0xff;
	i=i>>8;
	b1 = i&0xff;
	
	Send_SPI(b0);		
	
	Send_SPI(b1);		
	Send_SPI(b2);
	Send_SPI(b3);
	Send_SPI(b4);
	
	Send_SPI(b5);		// CRC
	count=0;
	do {				// R1 
		res=Send_SPI(0xFF);
		count=count+1;
	} while ( ((res&0x80)!=0x00)&&(count<0x0fff) );
	return res;
}
//------------------------------
static int WaitNotBusy()
{
	uint8_t Busy;
	do {
		Busy=Send_SPI(0xFF);
	} while (Busy!=0xFF);
  return 0;	
}	

//--------------------------------

uint16_t SD_init()
{
	long int count;
	uint8_t temp;
	uint8_t resp[4];
// step 1
		DelayMiliSec(100);
		SS_set;
		MOSI_set;
	
		for (uint8_t i=1; i<80; i++)
		{
			SCK_set;
			for (uint8_t k=0; k<70; k++) {}
			SCK_reset; 
			for (uint8_t k=0; k<25; k++) {}			
		}
		SS_reset;
	
	SPI_config();
		
// step 2
	//WaitNotBusy();
	temp=SD_cmd (0x40,0,0x95);	//CMD0 = 01 000000 
	if (temp!=0x01) return 0x40; 	// bad init
	
		PrintStrAndNumber("temp:", 5, temp);
// **************************************

// step 3	
	WaitNotBusy();
	count=0;

	//do{			
		temp=SD_cmd (0x40 | 0x08, 0x000001AA, (0x43<<1)|1);	//CMD8
		resp[0]=Send_SPI(0xFF);
		resp[1]=Send_SPI(0xFF);
		resp[2]=Send_SPI(0xFF);
		resp[3]=Send_SPI(0xFF);
		//if (temp==0x05) break;
	//} while ( (temp!=0x00)&&(count<0x0f) );
	
	if (temp==0x05)
	{
		count=0;
		do{				
			temp=SD_cmd (0x41,0,0x95);	//CMD1 = 01 000001
			count++;
		} while ( (temp!=0x00)&&(count<0xfff) );		//???? 0x01 ?????? R1
		if (temp==0x00) return TypeAdresBlok;
	}
	
// step 4
	WaitNotBusy();	
	
	uint8_t temp1=0;
	count=0;
	do{				
		temp1=SD_cmd(0x40|0x37, 0, 0x95);	//CMD55
		temp=SD_cmd (0x40|0x29, 0x40000000,(0x7F<<1)|1); //ACMD41
		count++;
	} while ( (temp!=0x00)&&(count<0x0fff) );		//???? 0x01 ?????? R1
//	if (temp!=0) return 0x40|0x37;
/*	
	if ((temp==0x01)&&(resp[3]==0xAA)) 
	{
		TypeCard=3;
	
		temp=SD_cmd (0x40|0x29, 0x40000000,(0x7F<<1)|1); //ACMD41
		if (temp!=0) return 0x40|0x29;
		
		temp=SD_cmd (0x40|0x3A, 0x00, 0x95); //CMD58
		if (temp!=0) return 0x40|0x3A;
		resp[0]=Send_SPI(0xFF);
		resp[1]=Send_SPI(0xFF);
		resp[2]=Send_SPI(0xFF);
		resp[3]=Send_SPI(0xFF);
	}	
	else 
	{	
		TypeCard=2;	//SDSC

		temp=SD_cmd (0x40|0x29, 0x00,(0x7F<<1)|1); //ACMD41
		if (temp!=0) return 0x40|0x29;	
		if (temp==0x04) //TypeCard=1; 
		{
			count=0;
			do{				
				temp=SD_cmd (0x41,0,0x95);	//CMD1 = 01 000001
				count++;
			} while ( (temp!=0x00)&&(count<0x0fff) );		//???? 0x01 ?????? R1
			if (temp==0) TypeCard=1; else return 0x41;
		}
	}*/
	temp=SD_cmd(0x40|0x10, 0x200, 0x95);	//CMD16
	if (temp!=0x00) return 0x10;
	
	temp=SD_cmd(0x40|0x3A, 0, 0x95);	//CMD58
	if (temp!=0x00) return 0x10;
	resp[0]=Send_SPI(0xFF);
	resp[1]=Send_SPI(0xFF);
	resp[2]=Send_SPI(0xFF);
	resp[3]=Send_SPI(0xFF);
	
	PrintStrAndNumber("R3:", 3, resp[3]);
	
	TypeAdresBlok=0x0001;
	return TypeAdresBlok;
}

//--------------------------------------------------------------------------------------
uint8_t WhaitDatatoken()
{
	uint16_t c=0;
	uint8_t temp;
	do{												//whait datatoken)
				temp=Send_SPI(0xff);
				c++;
	} while ( (temp!=0xfe)&&(c<0x0fff) );		// fe - datatoken
	return temp;
}

uint8_t CommandReadSector(uint32_t FirstByteOffset)
{
			uint8_t temp=SD_cmd(0x51, FirstByteOffset, 0x95);
	
			return WhaitDatatoken();
}

//--------------------------------------------------------------------------------------
//                   READ FUNCTIONS
//--------------------------------------------------------------------------------------

uint8_t ReadSector(uint32_t AdresSector, uint8_t *buf)
{
	__disable_irq ();
		uint8_t temp=CommandReadSector(AdresSector);
		if (temp==0xfe)
			for(uint16_t i=0; i<512; i++)	buf[i]=Send_SPI(0xff);

		Send_SPI(0xff);
		Send_SPI(0xff);
	__enable_irq ();
	return temp;	
}

//-------------------------------------------------------------------------------------
uint32_t ReadMBR()
{
	uint32_t StartPaticion;	
	char MSDOS[5]="MSDOS";
	uint8_t FlagBootSector=0;
	uint16_t count=0;
	StartPaticion=0;
	
	uint8_t temp=CommandReadSector(0);
	if (temp==0xfe)
				while (count<512)		
				{
					temp=Send_SPI(0xff);
					if ((count>=0x0003)&&(count<=0x0007)&&(MSDOS[count-0x0003]==temp)) FlagBootSector++;
					if ((count>=0x01C6)&&(count<0x01CA))	StartPaticion|=temp<<(8*(count-0x01C6));
					count++;
				}
		if (FlagBootSector==5) return 0; //Первый физ. сектор является загрузочным
				
		StartPaticion=StartPaticion*TypeAdresBlok;
		PrintStrAndNumber("StartPaticion:", 14, StartPaticion);		
		return StartPaticion;
}

uint32_t ReadBootSector()
{
	uint16_t ReservedSectors=0;
	uint32_t SectorPerFat=0;
	uint8_t NumberOfFats=0;
//	uint32_t BPB_RootClus=0;
	
	uint32_t StartPaticion = ReadMBR();
	uint8_t temp=CommandReadSector(StartPaticion);
			int count=0;
	if (temp==0xfe)
	{		
				count=0;	
				while (count<512)		
				{
					temp=Send_SPI(0xff);
					if (count==0x0D)	SectorPerCluster|=temp;
					if ((count>=0x0E)&&(count<=0x0F))	ReservedSectors|=temp<<(8*(count-0x0E));
					
					if ((count>=0x16)&&(count<=0x17))	{ SectorPerFat|=temp<<(8*(count-0x16)); TypeFat+=temp;} //Fat16
					if ((count>=0x24)&&(count<=0x27)&&(TypeFat==0))	SectorPerFat|=temp<<(8*(count-0x24)); //Fat32
					
					if (count==0x10)	NumberOfFats|=temp;
					if ((count>=0x11)&&(count<=0x12))	RootEntries|=temp<<(8*(count-0x11));
					
					if ((count>=44)&&(count<=47))	RootStart|=temp<<(8*(count-44)); // first cluster root catalog for fat32
					count++;
				}	
			Send_SPI(0xff);
			Send_SPI(0xff);	
	}
		FatStart= StartPaticion+ReservedSectors*TypeAdresBlok;
		LongFat=SectorPerFat*NumberOfFats*TypeAdresBlok;
		
		if (TypeFat!=0) RootStart=FatStart+LongFat; //fat16
		
		PrintStrAndNumber("ReservedSectors:", 16, ReservedSectors);
		PrintStrAndNumber("SectorPerFat:", 13, SectorPerFat);
		PrintStrAndNumber("SectorPerCluster:", 17, SectorPerCluster);
		PrintStrAndNumber("NumberOfFats:", 13, NumberOfFats);
		PrintStrAndNumber("FatStart:", 9, FatStart);
		PrintStrAndNumber("LongFat:", 8, LongFat);
		PrintStrAndNumber("RootStart:", 10, RootStart);
		PrintStrAndNumber("RootEntries:", 12,  RootEntries);
}

//------------------------------------------------------------
uint32_t ClasterToSector(uint32_t Claster)
{
	uint32_t AdressSector=0;
	if (TypeAdresBlok==0x200)
			AdressSector = FatStart+LongFat+(32*RootEntries)+((Claster-2)*SectorPerCluster*0x200);
	
	if ((TypeAdresBlok==0x0001)&&(TypeFat!=0)) //fat16
		{
			AdressSector = FatStart+LongFat+RootEntries/16+((Claster-2)*SectorPerCluster);
			if (RootEntries % 16 !=0) AdressSector++;
		}	
		
	if ((TypeAdresBlok==0x0001)&&(TypeFat==0)) //fat32
				AdressSector = FatStart+LongFat+((Claster-2)*SectorPerCluster);
	
	return 	AdressSector;
}

uint32_t NextClusterAdress(uint32_t LastCl)
{
	uint32_t SectorAdres;
	if (TypeFat==0) LastCl*=4; else LastCl*=2;  // number cluster --> number (offset) byte
	SectorAdres=FatStart+(LastCl/512)*TypeAdresBlok;
	
	uint32_t NewCluster=0;
	uint16_t c=0;
	//return ++LastCl;
	if (CommandReadSector(SectorAdres)==0xfe)
		while (c<512)		
		{
			if (TypeFat==0) //Fat32
					if (c==LastCl%512) {  //    %128
															NewCluster=Send_SPI(0xff)|Send_SPI(0xff)<<8|Send_SPI(0xff)<<16|Send_SPI(0xff)<<24; 
															c+=4;
															NewCluster&=0x0fffffff;
															}
					else {Send_SPI(0xff); Send_SPI(0xff); Send_SPI(0xff); Send_SPI(0xff); c+=4;}
			else	
					if (c==LastCl%512) {NewCluster=Send_SPI(0xff)|Send_SPI(0xff)<<8; c+=2;} //   % 256
					else {Send_SPI(0xff); Send_SPI(0xff); c+=2;}
		}
	Send_SPI(0xff); Send_SPI(0xff);
	return	NewCluster;
}

//------------------------------------------------------------------------------------------
int16_t pos(uint8_t *MainStr, int16_t LenM, uint8_t *Sub, int16_t LenS)
{
	int16_t j = 0;
	for (int16_t i = 0; i <= LenM - LenS; i++) // Пока есть возможность поиска
  {
		for (j = 0; MainStr[i + j] == Sub[j]; j++); // Проверяем совпадение посимвольно
	  if (j==LenS) return i;
  }	 
	return -1;
}
//--------------------------------------------------------------------------------------------

uint32_t NextAdresSectorFat32(uint32_t *cluster, uint16_t *offset)
{
	if (*offset>=SectorPerCluster)
	{
		*offset=0;
		*cluster=NextClusterAdress(*cluster);
		if (*cluster<0x0FFFFFFF) return ClasterToSector(*cluster); // адрес сектора начала следующего кластера
		else return 0;  // конец файла
	}
	else
	{
		*offset = *offset + TypeAdresBlok; 
		return *offset+ClasterToSector(*cluster);
	}		
}
//---------------------------------------------------------------------------------------

struct DescriptorFile FindFileInRoot(uint16_t REntries, char* NameF, int8_t LenStr) //NameF[0]=0xE5 -ищем пустую или удалённую запись
{
	uint32_t Claster=RootStart;
	uint16_t countSecInCluster=0; // offset in cluster;
	uint16_t CountEntries=0; //chetchic zapisey kataloga
	uint32_t AdresSector;
	
	struct DescriptorFile Rec = {0, 0, 0, 0, 0};
	struct RootRecord RR[16];
	
	if (TypeFat==0) AdresSector = ClasterToSector(RootStart); //Fat32	
	else AdresSector=RootStart; //fat16
		
	
//	PrintStr0("\r Test function FindFileInRoot:\r\0");
//	PrintStrAndNumber("SeeRootBegin AdresSector:", 25,  AdresSector);
	
	CountEntries=16*(REntries/16);
	while (Rec.BeginFile == 0)
	{
			uint8_t temp = ReadSector(AdresSector, (uint8_t*)RR);
	//		if (temp != 0xfe) PrintStrAndNumber("Read ERROR:", 11,  temp);
		
			do
			{	
				uint8_t i = CountEntries % 16;
				//PrintStrAndNumber("i:", 2,  i);
				if (
						(  (RR[i].Attribut == 0x20)
						&& (pos(RR[i].NameFile, 11, NameF, LenStr)!=-1)
						&& (RR[i].NameFile[0]!=0xE5)
						&& (REntries <= CountEntries))
						|| ((NameF[0]==0xE5)&&((RR[i].NameFile[0]==0xE5) || (RR[i].NameFile[0]==0)))
					 )
				{
					Rec.NumberRecord = CountEntries;
					for (uint8_t j=0; j<11; j++) 
							if (RR[i].NameFile[j]!=0) Rec.NameFile[j] = RR[i].NameFile[j];
					Rec.SizeF = RR[i].SiseInByte;
					Rec.BeginFile = RR[i].FirstClusterL | (RR[i].FirstClusterH<<16);
					return Rec;
				} 
			} while(++CountEntries % 16);
			
	//		PrintStrAndNumber("CountEntries:", 13,  CountEntries);
	//		PrintStrAndNumber("SeeRootBegin AdresSector:", 25,  AdresSector);
	//		PrintStrAndNumber("Claster:", 8,  Claster);
			if (TypeFat==0) //Fat32	
			{	
				AdresSector=NextAdresSectorFat32(&Claster, &countSecInCluster);
				if (AdresSector==0) {Rec.NameFile[0]='N'; Rec.NameFile[1]='O'; Rec.NameFile[2]='T'; Rec.NumberRecord=0; return Rec;}
			/*	if (++countSecInCluster>=SectorPerCluster)
				{
					countSecInCluster=0;
					Claster=NextClusterAdress(Claster);
					if (Claster>=0x0FFFFFFF) {Rec.NameFile[0]='N'; Rec.NameFile[1]='O'; Rec.NameFile[2]='T'; Rec.NumberRecord=0; return Rec;}
					AdresSector=ClasterToSector(Claster);
				}*/
			} else 	{ AdresSector+=TypeAdresBlok; if (CountEntries>RootEntries) return Rec; } //Fat16
 }
	//	PrintStr0("End test function FindFileInRoot:\r\0");
		return Rec; 
}

//----------------------------------------------------------------------------------
//                  WRITE FUNCTIONS
//----------------------------------------------------------------------------------
uint8_t WriteStr(uint32_t AdresBlock, uint16_t Pos ,uint8_t* Str, int16_t LenthStr)
{
		uint8_t Block[512];
		uint8_t r=0;
	
		if (CommandReadSector(AdresBlock)==0xfe)
		{
				for(uint16_t i=0; i<512; i++) Block[i]=Send_SPI(0xff);
				Send_SPI(0xff); Send_SPI(0xff);
		}
		
		if (LenthStr==-1) for (LenthStr=0; Str[LenthStr]; LenthStr++);
	
		if (SD_cmd(0x40|0x18, AdresBlock, 0x95)!=0) return 0x18;
		Send_SPI(0xff);
		Send_SPI(0xfe);
		for(uint16_t i=0; i<512; i++) 
			if ((i>=Pos)&&(i<Pos+LenthStr)) {Send_SPI(*Str); Str++;}
			else Send_SPI(Block[i]);
		Send_SPI(0xff); Send_SPI(0xff);
		r=Send_SPI(0xff); 	// recieve kod 0xE5 - Data Accepted
			
		for(uint16_t i=0; ((Send_SPI(0xff)!=0xff)&&(i<0xefff)); i++) {if (i>0xdfff) return 0xff;}
		
		return r;
}
//-----------------------------------------------------------------------------------
uint8_t WriteSector(uint32_t AdresBlock, uint8_t* Block)
{
		uint8_t r=0;
		
		__disable_irq ();
			if (SD_cmd(0x40|0x18, AdresBlock, 0x95)!=0) return 0x18;
			Send_SPI(0xff);
			Send_SPI(0xfe);
			for(uint16_t i=0; i<512; i++) Send_SPI(Block[i]);

			Send_SPI(0xff); Send_SPI(0xff);
			r=Send_SPI(0xff); 	// recieve kod 0xE5 - Data Accepted
			
			for(uint16_t i=0; ((Send_SPI(0xff)!=0xff)&&(i<0xefff)); i++) {if (i>0xdfff) return 0xff;}
		__enable_irq ();
		
		return r;
}
//---------------------------------------------------------------------
void IntSaveSector(uint32_t SectorAdres, uint16_t Offset, uint32_t Num)
{
	uint8_t V[4];
	V[0]=Num; V[1]=Num>>8; V[2]=Num>>16; V[3]=Num>>24;
	
	WriteStr(SectorAdres, Offset,  V, 4);
}	

//--------------------------------------------------------------------------
uint32_t FreeCluster(uint16_t offset)
{
	uint32_t SectorAdres;
	SectorAdres=FatStart+offset*TypeAdresBlok;
	uint32_t Cluster=1;
	uint32_t FreeCl=0;
	
	while (Cluster!=0)
	{	
		uint16_t c=0;
		if (CommandReadSector(SectorAdres)==0xfe)
		{	
			while(c<512)
			{
				if (TypeFat==0) //Fat32
				{Cluster=Send_SPI(0xff)|Send_SPI(0xff)<<8|Send_SPI(0xff)<<16|Send_SPI(0xff)<<24; Cluster&=0x0fffffff; c+=4;} 
				else	{ Cluster=Send_SPI(0xff)|Send_SPI(0xff)<<8;  c+=2; }
				
				if(Cluster==0) break;
				else FreeCl++;
			} 
			while(c<512) {Send_SPI(0xff); c++;}			
			Send_SPI(0xff); Send_SPI(0xff);
		}
		SectorAdres+=TypeAdresBlok;
	}				
		
	return	FreeCl;
}
//-------------------------------------------------------------------------------
void ClusterNewValue(uint32_t Claster, uint32_t Value)
{
	//uint32_t NewCluster = FreeCluster(0); 
	
	uint32_t SectorAdres;
	uint8_t V[4];
	V[0]=Value; V[1]=Value>>8; V[2]=Value>>16; V[3]=Value>>24;
	uint8_t sizeRecInFat;
	if (TypeFat==0) sizeRecInFat=4; else sizeRecInFat=2;  
	
	uint32_t OffsetByteCluster = Claster*sizeRecInFat; // number cluster --> number (offset) byte
	
	SectorAdres=FatStart+(OffsetByteCluster/512)*TypeAdresBlok;
	WriteStr(SectorAdres, OffsetByteCluster%512,  V, sizeRecInFat);

	SectorAdres=FatStart + (LongFat/2) + (OffsetByteCluster/512)*TypeAdresBlok; // NumberOfFats
	WriteStr(SectorAdres, OffsetByteCluster%512,  V, sizeRecInFat);
	
	return ;
}	

//----------------------------------------------------------------------------

uint16_t FreeRootRecord(uint32_t *ASector)
{
	uint32_t AdresSector;
	if (TypeFat==0) AdresSector = ClasterToSector(RootStart); //Fat32	
	else AdresSector=RootStart; //fat16
	
	uint16_t CountEntries=0;
	uint8_t buf[512];
	uint32_t cluster = RootStart; 
	uint16_t offsetInCluster=0;

	while (AdresSector!=0)
	{
		uint8_t temp = ReadSector(AdresSector, buf);
		for(uint8_t i=0; i<=16; i++) if ((buf[i*32]==0)||(buf[i*32]==0xE5)) {*ASector = AdresSector; return 32*i;}
			
		if (TypeFat==0)
		{	
				offsetInCluster++;
				if (offsetInCluster<SectorPerCluster) AdresSector++;
				else
						{
							offsetInCluster=0;
							//резервирование нового кластера
							uint32_t NewCl = FreeCluster(0);  // находим свободный кластер
							ClusterNewValue(cluster, NewCl);  // сохраняем номер нового кластера в старом
							cluster=NewCl;
							ClusterNewValue(cluster, 0x0FFFFFFF); // терминируем новый кластер
							AdresSector = ClasterToSector(cluster);
						}
			//AdresSector = NextAdresSectorFat32(&cluster, &offsetInCluster);	
		}	
	else { AdresSector+=TypeAdresBlok; if (CountEntries>=RootEntries)  AdresSector =0; } //Fat16
		
				PrintStrAndNumber("FreeRootRecord AdresSector:", 27,  AdresSector);
				PrintStrAndNumber("FreeRootRecord cluster:", 23,  cluster);
				PrintStrAndNumber("FreeRootRecord offsetInCluster:", 31, offsetInCluster);
		
		CountEntries+=16;
	}
	return 0;
}
//-------------------------------------------------------------------------
struct DescriptorFile CreateFile(char NameF[11])
{
	struct RootRecord Rec = {"DDDD    TXT", 0x20, 0x18, 0x00, 0x73CE, 0x5459, 0x5459, 0x0000, 0x7466, 0x5459, 0x0000, 0};
	struct DescriptorFile DF;
	uint32_t AdresSector; 
	
	uint32_t Cluster = FreeCluster(0);
	PrintStrAndNumber("Cluster:", 8,  Cluster);
	
	Rec.FirstClusterH = (0xFFFF0000&Cluster)>>16;
	Rec.FirstClusterL = 0x0000FFFF&Cluster;
	ClusterNewValue(Cluster, 0x0FFFFFFF); // резервируем первый кластер для файла
	
	//-------------- поиск пустой записи в root каталоге ---------------	
	
	DF.OffsetSectorRootRec = FreeRootRecord(&AdresSector);
	DF.AdresSectorRootRec=AdresSector;
	PrintStrAndNumber("AdresSectorRootRec:", 19,  DF.AdresSectorRootRec);
	PrintStrAndNumber("offset rec:", 11,  DF.OffsetSectorRootRec );
	//------------------------------------------------------------------
	 // = FindFileInRoot(0, "\0xE5\0xE5\0xE5", 3);	// ищем пустую запись в каталоге
	DF.BeginFile = Cluster;
	DF.Cluster  = Cluster;
	DF.Sector = 0; DF.SizeF = 0;
	
	if (NameF[0]!=0)
				for(uint8_t i=0; i<11; i++) Rec.NameFile[i]=NameF[i]; 
	else {Rec.NameFile[0]='F'; IntToStr(DF.Cluster, &Rec.NameFile[1]); Rec.NameFile[8]=NameF[1]; Rec.NameFile[9]=NameF[2]; Rec.NameFile[10]=NameF[3];	}
	for(uint8_t i=0; i<11; i++) DF.NameFile[i]=Rec.NameFile[i];
	
	WriteStr(DF.AdresSectorRootRec, DF.OffsetSectorRootRec,  (uint8_t*)&Rec, 32); // запись в каталоге
	
	DF.AdressSector =	ClasterToSector(DF.Cluster);	// первый сектор файла
	return DF;
}

//---------------------------------------------------------
void NextClusterForFile(struct DescriptorFile *DF)
{
		uint32_t NewCl = FreeCluster(0);
		ClusterNewValue(DF->Cluster, NewCl);
		DF->Cluster=NewCl;
		ClusterNewValue(DF->Cluster, 0x0FFFFFFF);		
}	

//----------------------------------------------------------
void WriteInFile(struct DescriptorFile *DF, uint8_t* Block)
{
	DF->AdressSector = ClasterToSector(DF->Cluster) + DF->Sector*TypeAdresBlok;
	WriteSector(DF->AdressSector, Block);
	DF->SizeF += 512;
	DF->Sector++;
	if (DF->Sector>=SectorPerCluster) 
	{
		DF->Sector=0; 
		NextClusterForFile(DF);
		IntSaveSector(DF->AdresSectorRootRec, 28+DF->OffsetSectorRootRec, DF->SizeF); // сохранение размера файла в rootRec
	}
}
