#include "mem.h"
#include <stdio.h>

int main() {
   printf("Starting mem testing...\r\n");
   int result = Mem_Init(32);
   if (result == 7) printf("Successfully called Mem_Init()\r\n");
   return 0;
}
