#include <stdio.h>
#include "mem.h"
#include "include.h"

int main() {
   printf("checking header size (%i should = 16)\r\n",(int)sizeof(struct header));
   printf("Starting mem testing...\r\n");
   int result = Mem_Init(6012);
   if (result == 7) printf("Successfully called Mem_Init()!\r\n");
   printf("Calling Mem_Alloc()... ");
   int *ptr = Mem_Alloc(2);
   printf("Got allocated memory @%p!\r\n",ptr);
   result = Mem_Free(ptr);
   if (result == 5) printf("Successfully called Mem_Free()!\r\n");
   printf("Calling Mem_Dump()... ");
   Mem_Dump();
   return 0;
}
