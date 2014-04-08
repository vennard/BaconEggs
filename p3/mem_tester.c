#include <stdio.h>
#include "mem.h"
#include "include.h"

int main() {
   printf("Starting mem testing...");
   /*
   int result = Mem_Init(6012);
   printf("Called Mem_Init... result=%i\r\n",result);
   if (result == 0) printf("Successfully called Mem_Init()!\r\n");
   int *ptr = Mem_Alloc(2);
   int *ptr2 = Mem_Alloc(14);
   int *ptr3 = Mem_Alloc(32);
   printf("Got allocated memory @%p!\r\n",ptr);
   result = Mem_Free(ptr);
   Mem_Dump();
   if (result == 0) printf("Successfully called Mem_Free()!\r\n");
   Mem_Free(ptr2);
   Mem_Free(ptr3);
   printf("Calling Mem_Dump()... ");
   printf("finishing up testing program.........\r\n");
   printf("---------------------------------------------------------\r\n");
   */
   printf("taking apart alloc2\r\n"); 
   int r1 = Mem_Init(4096);
   if (r1 != 0) printf("Mem_Init failed!\r\n");
   int *r2 = Mem_Alloc(8);
   printf("Called Mem_Alloc(8) with result = %p\r\n",r2);
   int *r3 = Mem_Alloc(16);
   printf("Called Mem_Alloc(16) with result = %p\r\n",r3);
   int *r4 = Mem_Alloc(32);
   printf("Called Mem_Alloc(32) with result = %p\r\n",r4);
   return 0;
}
