#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pageTable {
	int pageNumber;
	int counter;
} PageTable;

int getLRUFrameNum(PageTable* pageTable, int entry);

int main(int argc, char *argv[])
{
    FILE *storeFile;
	FILE *addressFile;
	char addressChar[10];
	int address;
	int low;
	int pageNumber;
	int offset;
	int pageIndex = 0;
	int frameNumber;
	int entry;
	int TLBnum = 0;
	int pageFault = 0;
	int TLBHit = 0;
	int count = 0;
	int entryNum = 0;
	int counter = 0;

	// receive console arguments
	entry = atoi(argv[1]);
    storeFile = fopen(argv[2],"rb");
	addressFile = fopen(argv[3],"r");

	PageTable pageTable[entry];
	int TLB_V[16];
	int TLB_P[16];
	signed char physicalMemory[entry][256]; 

	// page table, TLB, physical memory
	for(int i = 0; i < entry; i++){
		pageTable[i].pageNumber = -1;
		pageTable[i].counter = -1;
	}

	for(int i = 0; i < 16; i++){
		TLB_V[i] = -1;
		TLB_P[i] = -1;
	}

	for(int i = 0; i < entry; i++)
		for(int j = 0; j < 256; j++)
			physicalMemory[i][i] = 0;

	FILE *fp;

	if(entry == 256){ // 256 frames

		fp = fopen("output256.csv","w+");
		
		while (fgets(addressChar,sizeof(addressChar),addressFile) != NULL) {

			sscanf(addressChar, "%d", &address);

			// address -> low bits -> page number, offset
			low = address % 65536;
			pageNumber = low / 256;
			offset = low % 256;

			// check TLB
			int foundTLB = 0;
			for(int i = 0; i < 16; i++){
				if(TLB_V[i] == pageNumber){
					frameNumber = TLB_P[i];
					foundTLB = 1;
					break;
				}
			} 

			if(foundTLB == 0){ // page is not in TLB
				frameNumber = -1;
				for(int i = 0; i < entry; i++){ // search page table
					if(pageTable[i].pageNumber == pageNumber){
						frameNumber = i;
						break;
					}
				}
				if(frameNumber == -1){ // page is not in page table
					frameNumber = pageIndex;

					pageTable[entryNum].pageNumber = pageNumber;
					entryNum = entryNum + 1;

					pageIndex = pageIndex + 1;
					pageIndex = pageIndex % 256;

					pageFault = pageFault + 1;
					fseek(storeFile, 256*pageNumber,0);
					fread(physicalMemory[frameNumber],sizeof(signed char),256,storeFile);
				}

				// add to TLB
				TLB_V[TLBnum] = pageNumber;
				TLB_P[TLBnum] = frameNumber;
				TLBnum = TLBnum + 1;
				if(TLBnum == 16)
					TLBnum = 0;

			}
			else { // page is in TLB
				TLBHit = TLBHit + 1;
			}
			
			// logical address -> physical address
			// page number -> frame number
			// keep offset
			int pAddress = 0;
			pAddress = pAddress + frameNumber * 256;
			pAddress = pAddress + offset;
			
			signed char value = physicalMemory[frameNumber][offset];
			
			fprintf(fp,"%d,%d,%d\n", address, pAddress, value);

			count = count + 1;
    	}
	}
	else{ // 128 frames
		fp = fopen("output128.csv","w+");
		while (fgets(addressChar,sizeof(addressChar),addressFile) != NULL) {

			sscanf(addressChar, "%d", &address);

			// address -> low bits -> page number, offset
			low = address % 65536;
			pageNumber = low / 256;
			offset = low % 256;

			// check TLB
			int foundTLB = 0;
			for(int i = 0; i < 16; i++){
				if(TLB_V[i] == pageNumber){
					frameNumber = TLB_P[i]; // get frame number
					foundTLB = 1;
					
					// renew page table
					for(int j = 0; j < entry; j++){
						if(pageTable[j].pageNumber == pageNumber){
							pageTable[j].counter = counter;
							break;
						}
					}

					break;
				}
			} 
			if(foundTLB == 0){ // page is not in TLB
				frameNumber = -1;

				for(int i = 0; i < entry; i++){ // search page table
					if(pageTable[i].pageNumber == pageNumber){ // if the page number exists

						frameNumber = i; // get frame number

						// renew page table
						pageTable[i].counter = counter;
						break;
					}
				}				
				if(frameNumber == -1){ // page is not in page table

					if(pageTable[entry-1].pageNumber == -1){ // page table is not full
						frameNumber = entryNum;

						pageTable[entryNum].pageNumber = pageNumber;
						pageTable[entryNum].counter = counter;

						entryNum = entryNum + 1;
					}
					else{ // page table is full
						int lruFrameNum = getLRUFrameNum(pageTable, entry);
						frameNumber = lruFrameNum;
						pageTable[frameNumber].pageNumber = pageNumber; 
						pageTable[frameNumber].counter = counter; 

					}
				
					pageFault = pageFault + 1;
					fseek(storeFile,256*pageNumber,0);
					fread(physicalMemory[frameNumber],sizeof(signed char),256,storeFile);
				}

				// add to TLB
				TLB_V[TLBnum] = pageNumber;
				TLB_P[TLBnum] = frameNumber;
				TLBnum = TLBnum + 1;
				if(TLBnum == 16)
					TLBnum = 0;

			}
			else { // page is in TLB
				TLBHit = TLBHit + 1;
			}
			
			// logical address -> physical address
			// page number -> frame number
			// keep offset
			int pAddress = 0;
			pAddress = pAddress + frameNumber * 256;
			pAddress = pAddress + offset;
			
			signed char value = physicalMemory[frameNumber][offset];
			
			fprintf(fp,"%d,%d,%d\n", address, pAddress, value);

			count = count + 1;
			counter = counter + 1;
    	}
	}
    
	
	double pageFaultRate = ((double) pageFault / count) * 100;
	double TLBHitRate = ((double) TLBHit / count) * 100;

	fprintf(fp,"Page Faults Rate, %.2f\%%,\n", pageFaultRate);
	fprintf(fp,"TLB Hits Rate, %.2f\%%,", TLBHitRate);

	// close files
    fclose(storeFile);
	fclose(addressFile);
	fclose(fp);
    
    return 0;
}

int getLRUFrameNum(PageTable* pageTable, int entry){
	int minCounter = 2000;
	int lruFrameNum = -1;

	for(int i = 0; i < entry; i++){
		if(pageTable[i].counter < minCounter){
			minCounter = pageTable[i].counter;
			lruFrameNum = i;
		}		
	}
	return lruFrameNum;
}