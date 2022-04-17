#include "main.h"

#define Cluster_BUFF_SIZE 256

extern struct DescriptorFile RawFile;
extern uint32_t Clusters[256];
extern uint32_t FatAdress1;
extern uint32_t FatAdress2;

void InitADC();
void InitTimer();
void InitDMA();


void DMA1_Channel1_IRQHandler(void);
void StartIRQ();
void NewWavFile();
void SaveFile();

