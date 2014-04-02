#include <stdio.h>
#include "mem.h"
#include "include.h"

int main() {
   struct header h; 
   h.key = KEY;
   h.size = 9;
   if (h.key == KEY) printf("Tested header struct key variable... success!\r\n");
   if (h.size == 9) printf("Tested header struct size variable... success!\r\n");
   printf("Header struct size is %i must be less then 16 bytes!!!! \r\n",(int)sizeof(struct header));
   printf("Starting mem testing...\r\n");
   int result = Mem_Init(6012);
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
