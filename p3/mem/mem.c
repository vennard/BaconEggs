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

int *startaddress,*finaladdress;
int totalsize;
struct header head;
struct header *p_head;
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
   p_head = NULL;
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
      finaladdress = finalAddr;
      printf("************** Mem_Init was a success. startaddress is %p. and final address is %p ***************\r\n\r\n",startaddress,finalAddr);
   }
   close(fd);
   return 0;
}

//inserts a memory block into the linked list
//insert in ordered manner aka based on address
//ptr = address of new block (already confirmed enough space at address)
int insert(struct header* ptr) {
   printf("Starting insert at %p... ",ptr);
   struct header *temp,*t;
   int *thead;
   thead =(int *) p_head;
   t = p_head;
   if ((ptr <= t)||(p_head == NULL)) { //space below header, insert as new head
      p_head = ptr;
      printf("inserting node as new head... ");
      if (numblocks < 1) {
         printf(" its the only block!\r\n");
         ptr->next = NULL;
      } else {
         ptr->next = p_head; //set current pointer to correct address TODO check timing with write of newblock
      }
      return 0;
   } else if (t->next == NULL) { //after head when there is only header
      t->next = ptr;
      ptr->next = NULL;
      printf("only head present --- adding behind head!!!!!\r\n");
      return 0;
   } else if (ptr <= (t->next)) { //after head
      ptr->next = t->next;
      t->next = ptr; 
      printf("inserting node after header!\r\n");
      return 0;
   } else {
      printf("searching through linked list...");
      temp = t->next; //starting at block past head
      while (temp->next != NULL) {
         if ((ptr > temp)&&(ptr < temp->next)) {
            //found location for new block after temp
            ptr->next = temp->next;
            temp->next = ptr;
            printf("found spot after %p and before %p\r\n",temp,temp->next); 
            return 0;
         }
         temp = temp->next;
      }
      //check for inserting after last block
      if ((ptr > temp)&&((int*)ptr < finaladdress)) {
         temp->next = ptr;
         ptr->next = NULL;
         printf(" found spot after last block\r\n");
         return 0;
      }
   }
   printf("Failed to find a place to insert the new block! FAILED\r\n");
   return -1; 
}

//removes a memory block from the linked list
//TODO add invalidate (aka change key when you remove)
int removeBlock(struct header* ptr) {
   struct header *temp,*prev,*thead;
   thead = p_head;
   printf("Starting remove method - removing at %p\r\n",ptr);
   if (p_head == ptr) { //removing head of list
      p_head = thead->next; 
      printf("Removed head of the list, new head at %p...",p_head);
      return 0; 
   } else {
      temp = thead->next; //starting at block past head
      prev = thead;
      printf("Searching through linked list...");
      while (temp->next != NULL) {
         if (temp == ptr) { //found block to remove
            prev->next = temp->next;
            temp->next = NULL;
            printf("removing block at %p\r\n",temp); 
            return 0;
         }
         prev = temp;
         temp = temp->next;
      }
      //check for inserting after last block
      if (ptr == temp) {
         prev->next = NULL;
         printf(" removed last block from list\r\n");
         return 0;
      }
   }
   printf("Failed to remove the block! FAILED\r\n");
   return -1;
}

//Finds smallest open block of memory that fits the size and returns the address
struct header* findsmallest(int size) {
   printf("Launching findsmallest function!\r\n");
   printf("TESTING START ADDRESS VALUES: KEY - %c, SIZE - %i, ADDR - %p\r\n\r\n",p_head->key,p_head->size,p_head);
   int count = 0;
   int smallest = totalsize; //set smallest to max
   struct header* smallestptr = NULL;
   int *taddr, *finaladdr;
   struct header *lastaddr;
   struct header *curr = p_head; 
   //Look above head
   long freespace = ((int*)p_head - startaddress) / 8;
   printf("Searching above head... ");
   printf("freespace = %i (p_head %p - startaddress %p)!\r\n",freespace,p_head,startaddress);
   if (freespace > size) {
      smallest = freespace;   
      smallestptr = (struct header *) startaddress;
   }
   printf("Starting searching through linked list... ");
   while ((curr->next != NULL)&&(curr->key == KEY)) {
      count++;
      printf("Looping through the list!\r\n"); 
      if (count == 1) { //account for address of head
         taddr = (int*)p_head + (curr->size*8);
      } else {
         taddr = (int*)curr + (curr->size*8);
      }
      finaladdr = (int *)curr->next;  
      freespace = (finaladdr - taddr) / 8;
      printf("INSIDE LOOP: freespace = %i (finaladdr %p - taddr %p)! count = %i\r\n",freespace,finaladdr,taddr,count);
      if ((freespace > size)&&(freespace < smallest)) {
         smallest = freespace;
         smallestptr = (struct header *) taddr;
         printf("found free space in loop!\r\n");
      } 
      lastaddr = curr->next;
      curr = curr->next; 
   }
   printf(" done searching linked list\r\n");
   //Look below head
   if (count == 0) {
      taddr = ((int*)p_head + (p_head->size*8)); 
   } else {
      printf("LASTADDR @%p has next = %p and key %c \r\n",lastaddr,lastaddr->next,lastaddr->key);
      /*
      if (lastaddr->key != KEY) { //prevent reading invalid last entry
         taddr = (int*) lastaddr; 
         printf("INVALID header at lastaddr %p !!!!!!\r\n",lastaddr);
      } else {
      */
         taddr = (int*)lastaddr + ((lastaddr->size)*8);
         printf("adding lastaddr %p + lastaddr->size %i = taddr %p !!!!!!\r\n",lastaddr,lastaddr->size,taddr);
   }
   freespace = (finaladdress - taddr) / 8;
   printf("Searching below last block... ");
   printf("freespace = %i (finaladdress %p - taddr %p)!\r\n",freespace,finaladdress,taddr);
   if ((freespace > size)&&(freespace < smallest)) {
      smallest = freespace;
      smallestptr = (struct header *) taddr;
      printf("found free space below last block!\r\n");
   }
   printf("FINISHING BESTFIT ALG - FOUND LOCATION AT %p!\r\n",smallestptr);
   return smallestptr;
}

void *Mem_Alloc(int size) {
   int i,foundindex;
   int fsize = size;
   struct header prev, curr, next, smallestfound, out;
   int smallestsize = totalsize;
   int *endOfCurr,*endOfFree,*outaddr;
   struct header *addr;
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
   //if (numblocks <= 0) {
   if (p_head == NULL) {
      printf("P_HEAD == NULL ------ Writing to head - at startaddress! (size = %i)\r\n",fsize);
      addr = (struct header *)startaddress;
      p_head = addr;
      /*
      head.key = KEY;
      head.size = fsize + sizeof(struct header);
      head.next = NULL;
      p_head = startaddress; //save address of head of linked list
      //copy struct over to allocated memory
      memcpy(startaddress,&head,sizeof(struct header));
      numblocks++;
      printf("Allocated NEW HEAD from EMPTY LIST - numblocks = %i, p_head = %p, size = %i\r\n\r\n",numblocks,p_head,head.size);
      return startaddress;
      */
   } else { //search for BEST_FIT location by finding existing blocks
      addr = findsmallest(fsize);
      if (addr == NULL) { 
         printf("No space found for allocating memory block!!\r\n\r\n");
         m_error = E_NO_SPACE;
         return NULL;
      } 
   }
   addr->key = KEY;
   addr->next = NULL;
   addr->size = fsize + sizeof(struct header);
   //out.key = KEY;
   //out.size = fsize + sizeof(struct header);
   //out.next = NULL;
   printf("writing new block to address %p w/ size %i!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n",addr,fsize + sizeof(struct header));
   //memcpy(addr,&out,sizeof(struct header));
   int check = insert(addr); //inserts and set next variable in header
   if (check == 0) {
      printf("Insert call was a success!\r\n");
      numblocks++;
   } else {
      printf("Insert call was a failure!FAILED !\r\n");
   }
   //allocate memory (save header) and return
   printf("NUMBLOCKS AT - %i\r\n",numblocks);
   printf("Finishing call to mem_alloc - returning %p!\r\n\r\n",addr);
   return addr;
}

int Mem_Free(void *ptr) {
   printf("Called MEM_FREE numblocks = %i\r\n",numblocks);
   int i;
   struct header rm,temp,prev,*t;
   if (ptr == NULL) {
      return -1;
   }
   if (numblocks <= 0) {
      printf("No blocks available to free!\r\n");
      return -1;
   }
   memcpy(&rm,ptr,sizeof(struct header));
   printf("trying to remove block with key %c, size %i!\r\n",rm.key,rm.size);
   if (rm.key != KEY) {
      printf("Found invalid key! Failed call to mem_free!\r\n");
      m_error = E_BAD_POINTER;
      return -1; //check for valid header
   }
   int check = removeBlock(ptr);
   if (check == 0) {
      printf("Remove call was a success!\r\n");
      numblocks--;
   } else {
      printf("Remove call was a failure! FAILED!\r\n");
   }
   if (numblocks < 0) numblocks = 0;
   return 0;
}

void printMemBlock(int size, int* ptr) {
   //int i;
   //int printsize = h.size >> 3; //reduce size of print out
   printf("--------------MEMORY BLOCK OF SIZE %i---------@%p-----\r\n",size,ptr);
   //for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF MEMORY BLOCK------------------\r\n\r\n");
}

void printFreeBlock(int size, int *addr) {
   //int i;
   //int printsize = size >> 3; //reduce size of print out
   printf("--------------FREE BLOCK OF SIZE %i------@%p--------\r\n",size,addr);
   //for (i = 0;i < printsize;i++) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
   printf("--------------END OF FREE BLOCK------------------\r\n\r\n");
}

void Mem_Dump() {
   printf("Calling Mem_Dump().................................\r\n\r\n");
   printf("--------startaddress %p ------------------- finaladdress %p ----------- \r\n\r\n",startaddress,finaladdress);
   struct header *temp;  
   struct header *thead;
   thead = p_head;
   int i,size,gap;
   if (totalsize <= 0) printf("Memory has not been initialized! Failed!\r\n");
   if (numblocks <= 0) printf("No memory has been mapped, full memory of size %i available.\r\n",totalsize);
   if (numblocks > 0) { //Navigate through linked list of mem and print each section (including free sections)
      if (((int*)p_head - startaddress) > 0) {
         printf("GAP ON TOP OF MEM STACK - size = %i\r\n",((int*)p_head - startaddress)/8);
      }
      printf("\r\n HEAD of list: size %i @ %p! ------------------------------>\r\n",thead->size,p_head);
      printf("--------------END OF MEMORY BLOCK------------------\r\n\r\n");
      temp = thead->next;
      while ((temp->next != NULL)&&(temp->key == KEY)) {
         printMemBlock(temp->size,(int*)temp);
         //printf("temp->next = %p - temp %p + temp->size %i *8\r\n",temp->next,&temp,temp->size);
         //gap = ((int*)&temp->next - ((int*)&temp + (temp->size*8)))/32;
         //if (gap > 0) printFreeBlock(gap,(int*)(&temp +(temp->size*8)));
         temp = temp->next;
      }
      printf("Last Block has address %p with size %i and next %p (%i)\r\n\r\n",temp,temp->size,temp->next,(int*)temp->next);
      printMemBlock(temp->size,(int*)temp);
      gap = (finaladdress - ((int*)temp + (temp->size*8)))/8;
      printf("LAST GAP IS size %i!!!\r\n",gap);
   }
}


