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
   Mem_Alloc(1);
   Mem_Alloc(5);
   Mem_Alloc(14);
   Mem_Alloc(8);
   Mem_Alloc(1);
   Mem_Alloc(4);
   Mem_Alloc(9);
   //Mem_Alloc(33);
   //Mem_Alloc(55);

   Mem_Dump();



   return 0;
}
