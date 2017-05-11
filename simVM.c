//
// A virtual memory simulation.
//
// Debajyoti Dash

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define VM_ROUNDROBIN_REPLACEMENT 0
#define VM_LRU_REPLACEMENT 1

struct vm_s{
	int data;
	float fdata;
	unsigned int adrs;
	struct pagetbl_s *pagetable;
	struct tlbtbl_s *tlbtable;
	struct controltbl_s *controltable;
	int pgfault;
	int tlbmiss;
	int diskwrites;
};
typedef struct vm_s vm_t;


struct pagetbl_s{
	unsigned int addr;
	bool mod;
	int pagetimestamp;
	bool diskw;
};
typedef struct pagetbl_s pagetbl_t;


struct tlbtbl_s{
	unsigned int vaddr;
	unsigned int paddr;
	int tlbtimestamp;
};
typedef struct tlbtbl_s tlbtbl_t;


struct controltbl_s{
	int tlb;
	int page;
};
typedef struct controltbl_s controltbl_t;

static int offset = 0; // contains the bits by which address moved right
static unsigned int sizePM_v; // size of physical memory
static unsigned int sizeTLB_v; // size of TLB
static char pageReplAlg_v; // Round-robin or LRU for Page table
static char tlbReplAlg_v; // Round-robin or LRU for TLB table
static int pgtablesize = 0; // size of page table
static int pgtabletimestamp = 0; // page table timestamp
static int tlbtabletimestamp = 0; // TLB table timestamp
static bool isfloat = false;

// createVM
//
void *createVM(
  unsigned int sizeVM,   // size of the virtual memory in pages
  unsigned int sizePM,   // size of the physical memory in pages
  unsigned int pageSize, // size of a page in words
  unsigned int sizeTLB,  // number of translation lookaside buffer entries
  char pageReplAlg,      // page replacement alg.: 0 is Round Robin, 1 is LRU
  char tlbReplAlg        // TLB replacement alg.: 0 is Round Robin, 1 is LRU
){

	offset = 0;
	sizePM_v = 0;
	sizeTLB_v = 0;
	pageReplAlg_v = NULL;
	tlbReplAlg_v = NULL;
	pgtablesize = 0;
	pgtabletimestamp = 0;
	tlbtabletimestamp = 0;

	vm_t *virtualmem = NULL;
	pagetbl_t *pagetbl = NULL;
	tlbtbl_t *tlbtbl = NULL;
	controltbl_t *controltbl = NULL;

	// Check if pageSize is power of 2
	unsigned int x = pageSize;
	while (((x & 1) == 0) && x > 1){
		x >>= 1;
	}

	// Check size of the virtual memory times the size of a page must be 
	// less than or equal to 2^32
	int twosuperscript = 0;
	int vmsize = sizeVM*pageSize;
	while(((vmsize & 1) == 0) && vmsize >1){
		twosuperscript++;
		vmsize >>= 1;
	}

	if (sizeVM < sizePM)
	{
		fprintf(stderr, "Virtual memory is smaller than physical memory\n");
		exit(-1);
	}
	else if (sizePM < 1)
	{
		fprintf(stderr, "Physical memory is less than zero\n");
		exit(-1);
	}
	else if(x != 1){
		fprintf(stderr, "Size of page is not power of 2\n");
		exit(-1);
	}
	else if (sizeTLB > sizePM)
	{
		fprintf(stderr, "Size of TLB greater than size of physical memory\n");
		exit(-1);
	}
	else if (sizeTLB < 1)
	{
		fprintf(stderr, "Size of TLB must be greater then zero\n");
		exit(-1);
	}
	else if(pageReplAlg != VM_LRU_REPLACEMENT && pageReplAlg != 1 &&
			pageReplAlg != VM_ROUNDROBIN_REPLACEMENT && pageReplAlg != 0){
		fprintf(stderr, "Invalid replacement algorithm for page table\n");
		exit(-1);
	}
	else if(tlbReplAlg != VM_LRU_REPLACEMENT && tlbReplAlg != 1 &&
			tlbReplAlg != VM_ROUNDROBIN_REPLACEMENT && tlbReplAlg != 0){
		fprintf(stderr, "Invalid replacement algorithm for TLB\n");
		exit(-1);
	}
	else if(twosuperscript > 31 || vmsize > 1073741823){
		fprintf(stderr, "size of the virtual memory times the size of a page must be less than or equal to 2^32\n");
		exit(-1);
	}


	virtualmem = calloc(sizeVM*pageSize,sizeof(vm_t));
	if (virtualmem == NULL)
	{
		fprintf(stderr, "Memory cannot be allocated Virtual memory\n");
		exit(-1);
	}

	pgtablesize = (sizeVM*pageSize)/sizePM;
	pagetbl = calloc(pgtablesize, sizeof(pagetbl_t));
	if (pagetbl == NULL)
	{
		fprintf(stderr, "Memory cannot be allocated for page table\n");
		exit(-1);
	}

	tlbtbl = calloc(sizeTLB, sizeof(tlbtbl_t));
	if (tlbtbl == NULL)
	{
		fprintf(stderr, "Memory cannot be allocated TLB table\n");
		exit(-1);
	}

	controltbl = calloc(1,sizeof(controltbl_t));
	if (controltbl == NULL)
	{
		fprintf(stderr, "Memory cannot be allocated Control table\n");
		exit(-1);
	}

	for (int i = 0; i < (sizeVM*pageSize); ++i)
	{
		virtualmem[i].data=0;
		virtualmem[i].fdata=0;
		virtualmem[i].adrs=i;
		virtualmem[0].pgfault = 0;
		virtualmem[0].tlbmiss = 0;
		virtualmem[0].diskwrites = 0;
	}

	sizePM_v = sizePM;
	sizeTLB_v = sizeTLB;
	pageReplAlg_v = pageReplAlg;
	tlbReplAlg_v = tlbReplAlg;

	// Get page offset in offset variable
	unsigned int off = pageSize;
	while(((off>>1) & 0xFFFF) > 0){
		offset++;
		off = off>>1;
	}
	
	for (int i = 0; i < pgtablesize; ++i)
	{
		if(i<sizePM){
			pagetbl[i].mod = true;
			pagetbl[i].addr = i;
			pagetbl[i].pagetimestamp = 0;
			pagetbl[i].diskw = false;
		}
		else{
			pagetbl[i].mod = false;
			pagetbl[i].addr = 999;
			pagetbl[i].pagetimestamp = 0;
			pagetbl[i].diskw = false;
		}
		
	}

	for (int i = 0; i < sizeTLB; ++i)
	{
		tlbtbl[i].vaddr = i;
		tlbtbl[i].paddr = i;
		tlbtbl[i].tlbtimestamp = 0;
	}

	controltbl[0].tlb = 0;
	controltbl[0].page = 0;

	virtualmem->pagetable = pagetbl;
	virtualmem->tlbtable = tlbtbl;
	virtualmem->controltable = controltbl;

	return virtualmem;
}

// readInt
//
int readInt(void *handle, unsigned int address){
	
	vm_t *virtualmem = NULL;
	pagetbl_t *pagetbl = NULL;
	tlbtbl_t *tlbtbl = NULL;
	controltbl_t *controltbl = NULL;

	virtualmem = handle;
	pagetbl = virtualmem->pagetable;
	tlbtbl = virtualmem->tlbtable;
	controltbl = virtualmem->controltable;
	
	unsigned int vpageno = address>>offset;
	for (int i = 0; i < sizeTLB_v; ++i)
	{
		unsigned int x = tlbtbl[i].vaddr;
		if(x == vpageno){
			// TLB table replacement in LRU
			if(tlbReplAlg_v){
				tlbtabletimestamp++;
				tlbtbl[i].tlbtimestamp = tlbtabletimestamp;
				int t = 0;
				int low = 0;
				while(t<(sizeTLB_v-1)){
					if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
						t++;
						continue;
					}
					else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
						t++;
						continue;
					}
					else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
						t++;
						continue; 
					}
					else
						low = t+1;

					t++;
				}
				controltbl[0].tlb = low;
			}
			
			virtualmem->pagetable = pagetbl;
			virtualmem->tlbtable = tlbtbl;
			virtualmem->controltable = controltbl;

			if(isfloat) return NULL;
			else return virtualmem[address].data;
		}
	}
	++virtualmem[0].tlbmiss;    // TLB Miss
	int pos = controltbl[0].tlb;
	tlbtbl[pos].vaddr = vpageno;
	
	if(pagetbl[vpageno].mod){
		
		tlbtbl[pos].paddr = pagetbl[vpageno].addr;

		// TLB table replacement in LRU
		if(tlbReplAlg_v){
			tlbtabletimestamp++;
			tlbtbl[pos].tlbtimestamp = tlbtabletimestamp;
			int t = 0;
			int low = 0;
			while(t<(sizeTLB_v-1)){
				if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
					t++;
					continue;
				}
				else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
					t++;
					continue;
				}
				else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
					t++;
					continue; 
				}
				else
					low = t+1;

				t++;
			}
			controltbl[0].tlb = low;
		}
		// TLB table replacement in Round-robin
		else{
			if(pos<(sizeTLB_v-1)) controltbl[0].tlb = pos+1;
			else controltbl[0].tlb = 0;
		}

		// Page table replacement in LRU
		if(pageReplAlg_v){
			pgtabletimestamp++;
			pagetbl[vpageno].pagetimestamp = pgtabletimestamp;
			int t = 0;
			while(!pagetbl[t].mod){
				t++;
			}
			int count11 = 0;
			int low = t;
			t = 0;
			while(count11<(sizePM_v-1)){
				while(!pagetbl[t].mod){
					t++;
				}
				int g = t;
				t++;
				while(!pagetbl[t].mod){
					t++;
				}
				int h = t;
				if(pagetbl[h].pagetimestamp > pagetbl[g].pagetimestamp){
					count11++;
					if(pagetbl[g].pagetimestamp < pagetbl[low].pagetimestamp)
						low = g;
					else if(pagetbl[g].pagetimestamp == pagetbl[low].pagetimestamp){
						if(pagetbl[g].addr < pagetbl[low].addr) low = g;
					}
					continue;
				}
				else if(pagetbl[h].pagetimestamp > pagetbl[low].pagetimestamp){
					count11++;
					low = g;
					continue;
				}
				else if(pagetbl[h].pagetimestamp == pagetbl[low].pagetimestamp){
					count11++;
					if(pagetbl[h].addr < pagetbl[low].addr) low = h;
					continue; 
				}
				else
					low = h;

				count11++;
			}
			controltbl[0].page = pagetbl[low].addr;
		}
		
		
		virtualmem->pagetable = pagetbl;
		virtualmem->tlbtable = tlbtbl;
		virtualmem->controltable = controltbl;

		if(isfloat) return NULL;
		else return virtualmem[address].data;
	}

	++virtualmem[0].pgfault; // Page Fault
	// Disk write in readInt
	if(!pagetbl[vpageno].mod){
		if(pagetbl[vpageno].diskw) ++virtualmem[0].diskwrites;
	}
	pagetbl[vpageno].mod = true;
	int pos1 = controltbl[0].page;
	
	tlbtbl[pos].paddr = pos1;
	int i = 0;
	unsigned int v = pagetbl[i].addr;
	while(v != pos1){
		i++;
		v = pagetbl[i].addr;
	}
	
	pagetbl[i].mod = false;
	pagetbl[i].addr = 999;
	if(pageReplAlg_v) pagetbl[i].pagetimestamp = 0;
	pagetbl[vpageno].addr = pos1;

	// Page table replacement in LRU
	if(pageReplAlg_v){
		pgtabletimestamp++;
		pagetbl[vpageno].pagetimestamp = pgtabletimestamp;
		int t = 0;
		while(!pagetbl[t].mod){
			t++;
		}
		int count11 = 0;
		int low = t;
		t = 0;
		while(count11<(sizePM_v-1)){
			while(!pagetbl[t].mod){
				t++;
			}
			int g = t;
			t++;
			while(!pagetbl[t].mod){
				t++;
			}
			int h = t;
			if(pagetbl[h].pagetimestamp > pagetbl[g].pagetimestamp){
				count11++;
				if(pagetbl[g].pagetimestamp < pagetbl[low].pagetimestamp)
					low = g;
				else if(pagetbl[g].pagetimestamp == pagetbl[low].pagetimestamp){
					if(pagetbl[g].addr < pagetbl[low].addr) low = g;
				}
				continue;
			}
			else if(pagetbl[h].pagetimestamp > pagetbl[low].pagetimestamp){
				count11++;
				low = g;
				continue;
			}
			else if(pagetbl[h].pagetimestamp == pagetbl[low].pagetimestamp){
				count11++;
				if(pagetbl[h].addr < pagetbl[low].addr) low = h;
				continue; 
			}
			else
				low = h;

			count11++;
		}
		controltbl[0].page = pagetbl[low].addr;
	}
	// Page table replacement in Round-robin
	else{
		if(pos1<(sizePM_v-1)) controltbl[0].page = pos1+1;
		else controltbl[0].page = 0;
	}
	
	// TLB table replacement in LRU
	if(tlbReplAlg_v){
		tlbtabletimestamp++;
		tlbtbl[pos].tlbtimestamp = tlbtabletimestamp;
		int t = 0;
		int low = 0;
		while(t<(sizeTLB_v-1)){
			if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
				t++;
				continue;
			}
			else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
				t++;
				continue;
			}
			else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
				t++;
				continue; 
			}
			else
				low = t+1;

			t++;
		}
		controltbl[0].tlb = low;
	}
	// TLB table replacement in Round-robin
	else{
		if(pos<(sizeTLB_v-1)) controltbl[0].tlb = pos+1;
		else controltbl[0].tlb = 0;
	}

	virtualmem->pagetable = pagetbl;
	virtualmem->tlbtable = tlbtbl;
	virtualmem->controltable = controltbl;

	if(isfloat) return NULL;
	else return virtualmem[address].data;
}

// readFloat
//
float readFloat(void *handle, unsigned int address){
	isfloat = true;
	vm_t *virtualmem;
	virtualmem = handle;
	readInt(virtualmem,address);
	float m = virtualmem[address].fdata;
	isfloat = false;
	return m;
}

// writeInt
//
void writeInt(void *handle, unsigned int address, int value){

	vm_t *virtualmem;
	pagetbl_t *pagetbl;
	tlbtbl_t *tlbtbl;
	controltbl_t *controltbl;

	virtualmem = handle;
	pagetbl = virtualmem->pagetable;
	tlbtbl = virtualmem->tlbtable;
	controltbl = virtualmem->controltable;
	unsigned int vpageno = address>>offset;
	for (int i = 0; i < sizeTLB_v; ++i)
	{
		unsigned int x = tlbtbl[i].vaddr;
		if(x == vpageno){
			// TLB table replacement in LRU
			if(tlbReplAlg_v){
				tlbtabletimestamp++;
				tlbtbl[i].tlbtimestamp = tlbtabletimestamp;
				int t = 0;
				int low = 0;
				while(t<(sizeTLB_v-1)){
					if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
						t++;
						continue;
					}
					else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
						t++;
						continue;
					}
					else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
						t++;
						continue; 
					}
					else
						low = t+1;

					t++;
				}
				controltbl[0].tlb = low;
			}

			virtualmem->pagetable = pagetbl;
			virtualmem->tlbtable = tlbtbl;
			virtualmem->controltable = controltbl;

			virtualmem[address].adrs = address;

			//if(isfloat) virtualmem[address].data = (float) value;
			if(isfloat) return NULL;
			else virtualmem[address].data = value;
			

			
			return NULL;
		}
	}
	++virtualmem[0].tlbmiss;    // TLB Miss
	int pos = controltbl[0].tlb;
	tlbtbl[pos].vaddr = vpageno;
	
	if(pagetbl[vpageno].mod){
		tlbtbl[pos].paddr = pagetbl[vpageno].addr;
		
		// TLB table replacement in LRU
		if(tlbReplAlg_v){
			tlbtabletimestamp++;
			tlbtbl[pos].tlbtimestamp = tlbtabletimestamp;
			int t = 0;
			int low = 0;
			while(t<(sizeTLB_v-1)){
				if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
					t++;
					continue;
				}
				else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
					t++;
					continue;
				}
				else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
					t++;
					continue; 
				}
				else
					low = t+1;

				t++;
			}
			controltbl[0].tlb = low;
		}
		// TLB table replacement in Round-robin
		else{
			if(pos<(sizeTLB_v-1)) controltbl[0].tlb = pos+1;
			else controltbl[0].tlb = 0;
		}

		// Page table replacement in LRU
		if(pageReplAlg_v){
			pgtabletimestamp++;
			pagetbl[vpageno].pagetimestamp = pgtabletimestamp;
			int t = 0;
			while(!pagetbl[t].mod){
				t++;
			}
			int count11 = 0;
			int low = t;
			t = 0;
			while(count11<(sizePM_v-1)){
				while(!pagetbl[t].mod){
					t++;
				}
				int g = t;
				t++;
				while(!pagetbl[t].mod){
					t++;
				}
				int h = t;
				if(pagetbl[h].pagetimestamp > pagetbl[g].pagetimestamp){
					count11++;
					if(pagetbl[g].pagetimestamp < pagetbl[low].pagetimestamp)
						low = g;
					else if(pagetbl[g].pagetimestamp == pagetbl[low].pagetimestamp){
						if(pagetbl[g].addr < pagetbl[low].addr) low = g;
					}
					continue;
				}
				else if(pagetbl[h].pagetimestamp > pagetbl[low].pagetimestamp){
					count11++;
					low = g;
					continue;
				}
				else if(pagetbl[h].pagetimestamp == pagetbl[low].pagetimestamp){
					count11++;
					if(pagetbl[h].addr < pagetbl[low].addr) low = h;
					continue; 
				}
				else
					low = h;

				count11++;
			}
			controltbl[0].page = pagetbl[low].addr;
		}

		virtualmem->pagetable = pagetbl;
		virtualmem->tlbtable = tlbtbl;
		virtualmem->controltable = controltbl;

		virtualmem[address].adrs = address;	
		//if(isfloat) virtualmem[address].data = (float) value;
		if(isfloat) return NULL;
		else virtualmem[address].data = value;
		

		
		return NULL;
	}

	++virtualmem[0].pgfault;    // Page Fault
	++virtualmem[0].diskwrites; // Disk write in writeInt
	pagetbl[vpageno].mod = true;
	pagetbl[vpageno].diskw = true;
	int pos1 = controltbl[0].page;
	
	tlbtbl[pos].paddr = pos1;
	int i = 0;
	unsigned int v = pagetbl[i].addr;
	while(v != pos1){
		i++;
		v = pagetbl[i].addr;
	}
	pagetbl[i].mod = false;
	pagetbl[i].diskw = false;
	pagetbl[i].addr = 999;
	if(pageReplAlg_v) pagetbl[i].pagetimestamp = 0;
	pagetbl[vpageno].addr = pos1;
	
	// Page table replacement in LRU
	if(pageReplAlg_v){
		pgtabletimestamp++;
		pagetbl[vpageno].pagetimestamp = pgtabletimestamp;
		int t = 0;
		while(!pagetbl[t].mod){
			t++;
		}
		int count11 = 0;
		int low = t;
		t = 0;
		while(count11<(sizePM_v-1)){
			while(!pagetbl[t].mod){
				t++;
			}
			int g = t;
			t++;
			while(!pagetbl[t].mod){
				t++;
			}
			int h = t;
			if(pagetbl[h].pagetimestamp > pagetbl[g].pagetimestamp){
				count11++;
				if(pagetbl[g].pagetimestamp < pagetbl[low].pagetimestamp)
					low = g;
				else if(pagetbl[g].pagetimestamp == pagetbl[low].pagetimestamp){
					if(pagetbl[g].addr < pagetbl[low].addr) low = g;
				}
				continue;
			}
			else if(pagetbl[h].pagetimestamp > pagetbl[low].pagetimestamp){
				count11++;
				low = g;
				continue;
			}
			else if(pagetbl[h].pagetimestamp == pagetbl[low].pagetimestamp){
				count11++;
				if(pagetbl[h].addr < pagetbl[low].addr) low = h;
				continue; 
			}
			else
				low = h;

			count11++;
		}
		controltbl[0].page = pagetbl[low].addr;
	}
	// Page table replacement in Round-robin
	else{
		if(pos1<(sizePM_v-1)) controltbl[0].page = pos1+1;
		else controltbl[0].page = 0;
	}
	
	// TLB table replacement in LRU
	if(tlbReplAlg_v){
		tlbtabletimestamp++;
		tlbtbl[pos].tlbtimestamp = tlbtabletimestamp;
		int t = 0;
		int low = 0;
		while(t<(sizeTLB_v-1)){
			if(tlbtbl[t+1].tlbtimestamp > tlbtbl[t].tlbtimestamp){
				t++;
				continue;
			}
			else if(tlbtbl[t+1].tlbtimestamp > tlbtbl[low].tlbtimestamp){
				t++;
				continue;
			}
			else if(tlbtbl[t+1].tlbtimestamp == tlbtbl[low].tlbtimestamp){
				t++;
				continue; 
			}
			else
				low = t+1;

			t++;
		}
		controltbl[0].tlb = low;
	}
	// TLB table replacement in Round-robin
	else{
		if(pos<(sizeTLB_v-1)) controltbl[0].tlb = pos+1;
		else controltbl[0].tlb = 0;
	}

	virtualmem->pagetable = pagetbl;
	virtualmem->tlbtable = tlbtbl;
	virtualmem->controltable = controltbl;

	virtualmem[address].adrs = address;
	//if(isfloat) virtualmem[address].data = (float) value;
	if(isfloat) return NULL;
	else virtualmem[address].data = value;
	

	
	return NULL;
	
}

// writeFloat
//
void writeFloat(void *handle, unsigned int address, float value){
	isfloat = true;
	vm_t *virtualmem;
	virtualmem = handle;
	writeInt(virtualmem,address,value);
	virtualmem[address].fdata = value;
	isfloat = false;
}

// printStatistics
//
void printStatistics(void *handle)
{
	vm_t *virtualmem;
	virtualmem = handle;
	printf("Number of page faults: [%d]\n", virtualmem[0].pgfault);
	printf("Number of TLB misses: [%d]\n", virtualmem[0].tlbmiss);
	printf("Number of disk writes: [%d]\n", virtualmem[0].diskwrites);

}

// cleanupVM
//
void cleanupVM(void *handle)
{
	vm_t *virtualmem;
	virtualmem = handle;

	free(virtualmem->pagetable);
	free(virtualmem->tlbtable);
	free(virtualmem->controltable);
	free(virtualmem);

	offset = 0;
	sizePM_v = 0;
	sizeTLB_v = 0;
	pageReplAlg_v = NULL;
	tlbReplAlg_v = NULL;
	pgtablesize = 0;
	pgtabletimestamp = 0;
	tlbtabletimestamp = 0;
}

