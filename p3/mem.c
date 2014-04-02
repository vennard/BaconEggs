#include "mem.h"
#include <stdio.h>

int Mem_Init(int sizeOfRegion) {
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

