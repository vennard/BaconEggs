#include "mem.h"
#include <stdio.h>

int main() {
   printf("Starting mem testing...\r\n");
   int result = Mem_Init(32);
   if (result == 7) printf("Successfully called Mem_Init()!\r\n");
   printf("Calling Mem_Alloc()... ");
   Mem_Alloc(32);
   int *ptr = 0;
   result = Mem_Free(ptr);
   if (result == 5) printf("Successfully called Mem_Free()!\r\n");
   printf("Calling Mem_Dump()... ");
   Mem_Dump();
   return 0;
}
