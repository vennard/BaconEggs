#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mem.h"
#include "include.h"

int *startaddress;
int totalsize;
struct header head;
void *p_head;
int numblocks;

/*
 * Called once by a process. Size requested must be rounded to page size. 
 * Function also sets global variables for linked list of headers
 * sizeOfRegion = bytes to request from mmap
 */
int Mem_Init(int sizeOfRegion) {
   //TODO if sizeOfRegion <= 0 then return -1 and set m_error = E_BAD_ARGS
   numblocks = 0;
   p_head = &head;
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
      printf("mmap call in Mem_Init was a success. startaddress is %p.\r\n",startaddress);
   }
   close(fd);
   return 7; //TODO change to 0
}

void *Mem_Alloc(int size) {
   int i;
   int fsize = size;
   if (size <= 0) {
      perror("Mem_Alloc size equal to or less then zero!");
      exit(1);
   } else {
      printf("Calling Mem_Alloc with size %i!\r\n",size);
   }
   //change size to be 8-byte aligned
   int modval = size % 8;
   if (modval != 0) {
      int diff = 8 - modval;  
      fsize = size + diff;
      printf("size not 8-byte aligned changing from %i to %i!\r\n",size,fsize);
   }
   //if no blocks implemented start new mem block at startaddress
   if (numblocks <= 0) {
      head.key = KEY;
      head.size = fsize + sizeof(struct header);
      head.next = NULL;
      //copy struct over to allocated memory
      memcpy(startaddress,&head,sizeof(struct header));
      numblocks++;
      printf("Successfully allocated first block of memory!\r\n");
      struct header t;
      memcpy(&t,startaddress,sizeof(struct header));
      printf("test read off allocated memory - key = %c, size = %i\r\n",t.key,t.size);
      return startaddress;
   } else { //search for BEST_FIT location by finding existing blocks
      for(i = 0;i < numblocks;i++) {
         if (i == 0) {
         }
      } 
   }
   return 0;
}

int Mem_Free(void *ptr) {
   return 5;
}

void printMemBlock(struct header h) {
   //int i;
   //int printsize = h.size >> 3; //reduce size of print out
   printf("--------------MEMORY BLOCK OF SIZE %i---------@%p-----\r\n",h.size,&h);
   //for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF MEMORY BLOCK------------------\r\n");
}

void printFreeBlock(int size, int *addr) {
   //int i;
   //int printsize = size >> 3; //reduce size of print out
   printf("--------------FREE BLOCK OF SIZE %i------@%p--------\r\n",size,addr);
   //for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF FREE BLOCK------------------\r\n");
}

void Mem_Dump() {
   struct header temp; 
   int i;
   printf("success!\r\n");
   if (totalsize <= 0) printf("Memory has not been initialized! Failed!\r\n");
   if (numblocks <= 0) printf("No memory has been mapped, full memory of size %i available.\r\n",totalsize);
   if (numblocks > 0) { //Navigate through linked list of mem and print each section (including free sections)
      if ((&p_head - (long) startaddress) > 0) {
         printf("Found gap inbetween start of allocated memory and first block of mem!\r\n");
         printFreeBlock((long)(&head - *startaddress),startaddress);
      }
      temp = head;
      for (i = 0;i < numblocks;i++) {
         printMemBlock(temp);
         if (temp.next != NULL) {
            //check for gap
            long endaddr = ((long)&temp + sizeof(struct header) + (temp.size*8));
            int gap = (long)&temp.next - endaddr;
            if (gap > 0) {
               printFreeBlock(gap, (void *) endaddr);
            }
            temp = *temp.next;
         } else {
            printf("encountered null pointer in linked list!\r\n");
         }
      }
   }
}

