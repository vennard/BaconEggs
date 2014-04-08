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
int *p_head;
int numblocks;
int block = 0;
int m_error;

/*
 * Called once by a process. Size requested must be rounded to page size. 
 * Function also sets global variables for linked list of headers
 * sizeOfRegion = bytes to request from mmap
 */
int Mem_Init(int sizeOfRegion) {
   //Can only be called once
   if (block == 1) { 
      m_error = E_BAD_ARGS;
      return -1;
   }
   block = 1;
   //must call valid region size
   if (sizeOfRegion <= 0) {
      m_error = E_BAD_ARGS;
      return -1;
   }
   numblocks = 0;
   int diff;
   int pagesize = getpagesize(); //page size in bytes  
   int modval = sizeOfRegion % pagesize;
   //must be page aligned memory request - round up bad requests
   if (modval != 0) {
      diff = pagesize - modval;
      printf("Requested size is not factor of page size (%i) - off by %i!\r\n",pagesize,diff);
   } else diff = 0;
   int size = sizeOfRegion + diff;
   printf("Total size requested is %i.\r\n",size);
   //Using Unix Hints method from the project specifications
   int fd = open("/dev/zero",O_RDWR);
   startaddress = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);  
   totalsize = size;
   if (startaddress == MAP_FAILED) {
      printf("mmap failed!\r\n");
      exit(1);
   } else {
      int *finalAddr = startaddress + (totalsize*8);
      printf("************** Mem_Init was a success. startaddress is %p. and final address is %p\r\n",startaddress,finalAddr);
   }
   close(fd);
   return 0;
}

void *Mem_Alloc(int size) {
   int i,foundindex;
   int fsize = size;
   struct header prev, curr, next, smallestfound, out;
   int smallestsize = size;
   int *endOfCurr,*endOfFree;
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
      printf("Writing to head - at startaddress! (size = %i)\r\n",fsize);
      head.key = KEY;
      head.size = fsize + sizeof(struct header);
      head.next = NULL;
      p_head = startaddress; //save address of head of linked list
      //copy struct over to allocated memory
      memcpy(startaddress,&head,sizeof(struct header));
      numblocks++;
      return startaddress;
   } else { //search for BEST_FIT location by finding existing blocks
      curr = head; 
      for(i = 0;i < numblocks;i++) {
         if (i == 0) { //first block
            //search for space above the head
            printf("CHECKING FOR SPACE ABOVE HEAD ( p_head - startaddress = %i > totalsize*8 = %i )\r\n",(int)(p_head - startaddress), totalsize*8);
            if (p_head - startaddress > (totalsize*8)) {
               printf("yes\r\n");
               //TODO allocate new head
            } else {
               //check for space below head
               //if (curr.next != NULL) {
               if (numblocks == 1) {
                  printf("Only head exists! ");
                  //next = *curr.next;
                  endOfCurr = p_head + (head.size*8); 
                  endOfFree = p_head + (totalsize*8);
                  }
                  /*
                  long freespace = (long)endOfFree - (long)endOfCurr;
                  //int freespace = 9;
                  printf("Checking (endOfFree=%p - endOfCurr=%p)=%i > fsize = %i! ",endOfFree,endOfCurr,(int)freespace,fsize);   
                  if ((freespace > fsize)&&(smallestsize > fsize)) { //found large enough smallest block
                     printf("new smallest block found! newsize=%i vs oldsize=%i!\r\n",(int)freespace,smallestsize);
                     smallestsize = freespace;
                     smallestfound = curr;
                  }
               } else {
                  printf("Checking space in between head and 2nd block!");
                  next = *curr.next;
               } 
               */
            }
            prev = head; 
         } else if (i == numblocks-1) { //last block
            curr = *prev.next;
            endOfCurr = (int *)&curr + curr.size; //get address of end of last mem block
            int *finalAddr = (int *)p_head + (totalsize*8);
            if (finalAddr - endOfCurr > fsize) { //check if there is space
               if (finalAddr - endOfCurr < smallestsize) {
                  smallestsize = finalAddr - endOfCurr;
                  smallestfound = curr; 
                  foundindex = i;
               }
            } 
         } else { //middle blocks
            curr = *prev.next;
            endOfCurr = (int *)&curr + curr.size; //get address of end of last mem block
            int space = (int *)&curr.next - endOfCurr;
            if ((space >= fsize)&&(space < smallestsize)) { //found smaller block
               smallestsize = space;
               smallestfound = curr;
               foundindex = i;
            } 
         }
      } 
      //Select and write to the smallestblock, unless NULL
      if (smallestsize == size) {
         printf("No space found for allocating memory block!!\r\n");
         m_error = E_NO_SPACE;
         return NULL;
      } else {
         endOfCurr = (int *)&smallestfound + smallestfound.size;
         out.key = KEY;
         out.size = fsize + sizeof(struct header);
         printf("Writing to @%p with free space %i (block is %i bytes)\r\n",endOfCurr,smallestsize,fsize);
         //insert out into the linked list
         prev = head;
         for (i = 0;i < numblocks;i++) {
            if (prev.next == &smallestfound) { //found header to are going to insert after 
               curr = *prev.next;
               out.next = curr.next; //insert link
               curr.next = &out; 
               break;
            } else {
               prev = *prev.next;
            }
         }
         //allocate memory (save header) and return
         memcpy(endOfCurr,&out,sizeof(struct header));
         numblocks++;
         return endOfCurr;
      }
   }
   return 0;
}

int Mem_Free(void *ptr) {
   int i;
   struct header rm,temp;
   if (ptr == NULL) {
      return -1;
   }
   memcpy(&rm,ptr,sizeof(struct header));
   printf("trying to remove block with key %c, size %i!\r\n",rm.key,rm.size);
   //check for removing head
   if (ptr == p_head) {
      printf("ohh so you want to remove the head of the list... well fine!\r\n");
      if (numblocks > 1) { 
         temp = *head.next;
      }
      head = temp;
      numblocks--;
   } else {
      temp = *head.next; 
      for (i = 0;i < numblocks;i++) {
         if(ptr == &temp) {
            //found block -- remove
         }
      }
   }
   return 0;
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
      if ((p_head - startaddress) > 0) {
         printf("Found gap inbetween start of allocated memory and first block of mem!\r\n");
         printFreeBlock((long)(p_head - *startaddress),startaddress);
      }
      temp = head;
      struct header prev;
      for (i = 0;i < numblocks;i++) {
         prev = temp;
         printMemBlock(temp);
         if (i == 0) {
            //check for gap
            long endaddr = ((long)p_head + sizeof(struct header) + (temp.size*8));
            if (temp.next != NULL) {
               int gap = (long)&temp.next - endaddr;
               printf("found gap of %i after mem block %i!\r\n",gap,i);
               if (gap > 0) {
                  printFreeBlock(gap, (void *) endaddr);
               }
            temp = *temp.next;
            }
         }
      }
      //print remaining free area (unit startaddress + totalsize) TODO might have to adjust for bytes
      int *endaddr = startaddress + totalsize;
      int *tptr;
      if (numblocks > 1) {      
         tptr = (int *)prev.next + sizeof(struct header) + (temp.size*8);
      } else {
         tptr = p_head + sizeof(struct header) + head.size;
      }
      int sizeleft = (endaddr - tptr);
      if (sizeleft > 0) {
         printFreeBlock(sizeleft,tptr);
      } 
   }
}


