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
int *p_head,*p_insert;
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

//Finds smallest open block of memory that fits the size and returns the address
int* findsmallest(int size) {
   printf("Launching findsmallest function!\r\n");
   int count = 0;
   int smallest = totalsize; //set smallest to max
   int* smallestptr = NULL;
   struct header curr = head; 
   //Look above head
   long freespace = p_head - startaddress;
   printf("Searching above head!\r\n");
   printf("freespace = %i (p_head %p - startaddress %p)!\r\n",freespace,p_head,startaddress);
   if (freespace > size) {
      smallest = freespace;   
      smallestptr = startaddress;
      p_insert = startaddress;
   }
   printf("Starting searching through linked list!\r\n");
   while (curr.next != NULL) {
      count++;
      printf("Looping through the list!\r\n"); 
      //TODO add
      curr = *curr.next; 
   }
   //Look below head
   int *finaladdr = startaddress + (totalsize*8);
   int *taddr;
   if (count == 0) {
      taddr = (p_head+ (head.size*8)); 
   } else {
      taddr = (int*)&curr + (curr.size*8);
   }
   freespace = (finaladdr - taddr) / 8;
   printf("Searching below last block!\r\n");
   printf("freespace = %i (finaladdr %p - &curr %p)!\r\n",freespace,finaladdr,taddr);
   if ((freespace > size)&&(freespace < smallest)) {
      smallest = freespace;
      smallestptr = taddr;
      p_insert = taddr;
      printf("found free space below last block!\r\n");
   }
   return smallestptr;
}
void *Mem_Alloc(int size) {
   int i,foundindex;
   int fsize = size;
   struct header prev, curr, next, smallestfound, out;
   int smallestsize = totalsize;
   int *endOfCurr,*endOfFree,*outaddr;
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
      int *addr = findsmallest(fsize);
      if (addr == NULL) { 
         printf("No space found for allocating memory block!!\r\n");
         m_error = E_NO_SPACE;
         return NULL;
      } else {
         out.key = KEY;
         out.size = fsize + sizeof(struct header);
         printf("writing new block to address %p w/ size %i!\r\n",addr,fsize);
         //insert out into the linked list -- can't be new head (handled earlier)
         printf("starting insert to linked list ------------------");
         curr = head;
         int count = 0;
         while (curr.next != NULL) {
            count++;
            if ((int*)&curr == p_insert) { //found header to insert after 
               printf("Found place to insert new block!\r\n");
               out.next = curr.next; //insert link
               curr.next = (struct header *)outaddr; 
               break;
            }
            curr = *curr.next;
         }
         if (count == 0) { //insert after head
            printf("inserting block after the head\r\n");
            head.next = (struct header *) addr;  
            out.next = NULL;
         }
         //allocate memory (save header) and return
         memcpy(addr,&out,sizeof(struct header));
         return addr;
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


