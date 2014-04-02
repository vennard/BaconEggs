#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mem.h"
#include "include.h"

int *startaddress;
int totalsize;
struct header head;

/*
 * Called once by a process. Size requested must be rounded to page size. 
 * Function also sets global variables for linked list of headers
 * sizeOfRegion = bytes to request from mmap
 */
int Mem_Init(int sizeOfRegion) {
   //TODO if sizeOfRegion <= 0 then return -1 and set m_error = E_BAD_ARGS
   head.size = 0; //initialize linked list 
   int diff;
   int pagesize = getpagesize(); //page size in bytes  
   int modval = sizeOfRegion % pagesize;
   if (modval != 0) {
      diff = pagesize - modval;
      printf("Requested size is not factor of page size (%i) - off by %i!\r\n",pagesize,diff);
   } else diff = 0;
   int size = sizeOfRegion + diff;
   printf("Total size requested is %i.\r\n",size);
   //Using Unix Hints method from the project specifications
   int fd = open("/dev/zero",O_RDWR);
   //set globals for memory
   startaddress = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);  
   totalsize = size;
   if (startaddress == MAP_FAILED) {
      //TODO return -1 and set m_error = E_BAD_ARGS
      printf("mmap failed!\r\n");
      exit(1);
   } else {
      printf("mmap call in Mem_Init was a success. Head is at %p.\r\n",startaddress);
   }
   close(fd);
   return 7; //TODO change to 0
}

void *Mem_Alloc(int size) {
   printf("success!\r\n");
   return 0;
}

int Mem_Free(void *ptr) {
   return 5;
}

void printMemBlock(struct header h) {
   int i;
   int printsize = h.size >> 3; //reduce size of print out
   //int *endaddr = (void *) h.size + &h;
   printf("--------------MEMORY BLOCK OF SIZE %i---------@%p-----\r\n",h.size,&h);
   for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF MEMORY BLOCK------------------\r\n");
}

void printFreeBlock(int size, int *addr) {
   int i;
   int printsize = size >> 3; //reduce size of print out
   int *endaddr = size + addr;
   printf("--------------FREE BLOCK OF SIZE %i---------@%p-----\r\n",size,addr);
   for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF FREE BLOCK------@%p------------\r\n",endaddr);
}

void Mem_Dump() {
   struct header temp; 
   int i = 0;
   printf("success!\r\n");
   if (totalsize <= 0) printf("Memory has not been initialized! Failed!\r\n");
   if (head.size == 0) printf("No memory has been mapped, full memory of size %i available.\r\n",totalsize);
   temp.size = 32;
   printMemBlock(temp);
   //printFreeBlock(16, 12);
   if (head.size > 0) { //Navigate through linked list of mem and print each section (including free sections)
      if (i == 0) {
         temp = head;
         //TODO check if there is space between startaddress and first header
      }
      
   }
   
}

