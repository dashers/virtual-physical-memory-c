//
// simple test of the simulated virtual memory system
//

#include <stdio.h>
#include <stdlib.h>
#include "simVM.h"

#define VMEMSIZE 64
#define PMEMSIZE 4
#define PAGESIZE 4
#define TLBSIZE 2
#define MEMSIZE (VMEMSIZE*PAGESIZE)

#define LRU VM_LRU_REPLACEMENT
#define RR VM_ROUNDROBIN_REPLACEMENT

int main(void)
{
  void *h = createVM(VMEMSIZE+1, PMEMSIZE, PAGESIZE, TLBSIZE, 0, 0);
  if (h == NULL)
  {
    fprintf(stderr, "createVM failed!\n");
    exit(-1);
  }

  int i, sum, sum2;

  // initialize the array
  for (i = 0; i < MEMSIZE; i++)
  {
    writeInt(h, i, i);
  }

  // now sum it
  sum = 0;
  sum2 = 0;
  for (i = 0; i < MEMSIZE; i++)
  {
    int tmp = readInt(h, i);
     //printf("# %d %d\n", tmp, i);
    sum += tmp;
    sum2 += i;
  }
  printf("sum is %d (should be %d)\n", sum, sum2);

  // print stats
  printf("\n");
  printStatistics(h);
  printf("\n");

  // cleanup
  cleanupVM(h);

  return 0;
}
