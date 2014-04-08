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
   printf("taking apart free2\r\n"); 

   Mem_Init(4096);
   void* ptr[4];

   ptr[0] = Mem_Alloc(8);
   ptr[1] = Mem_Alloc(16);
   int a = Mem_Free(ptr[0]); 
   int b = Mem_Free(ptr[1]);
   printf("COMPLETED 2 FREES a = %i, b = %i\r\n",a,b);
   ptr[2] = Mem_Alloc(32);
   ptr[3] = Mem_Alloc(8);
   int c = Mem_Free(ptr[2]);
   int d = Mem_Free(ptr[3]);
   printf("COMPLETED 2 FREES c = %i, d = %i\r\n",c,d);

   return 0;
}
