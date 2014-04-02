#include <stdio.h>
#include <unistd.h>
#include "mem.h"
#include "include.h"

/*
 * Called once by a process. Size requested must be rounded to page size. 
 * Function also sets global variables for linked list of headers
 * sizeOfRegion = bytes to request from mmap
 */
int Mem_Init(int sizeOfRegion) {
   int diff;
   int pagesize = getpagesize(); //page size in bytes  
   int modval = sizeOfRegion % pagesize;
   if (modval != 0) {
      diff = pagesize - modval;
      printf("Requested size is not factor of page size (%i) - off by %i!\r\n",pagesize,diff);
   } else diff = 0;
   int size = sizeOfRegion + diff;
   printf("Total size requested is %i.\r\n",size);
   //pa = mmap(addr (0), size, PROT_WRITE, MAP_SHARED, fildes, offset) 
   //int pa = mmap(0, roundedSize, PROT_WRITE, MAP_SHARED, 8, 0);  
   return 7;
}

void *Mem_Alloc(int size) {
   printf("success!\r\n");
   return 0;
}

int Mem_Free(void *ptr) {
   return 5;
}

void Mem_Dump() {
   printf("success!\r\n");
}

