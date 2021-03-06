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

void *startaddress,*finaladdress;
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
   printf("PAGESIZE = %i\r\n",pagesize);
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
      finaladdress = startaddress + totalsize;
      int testsize = finaladdress - startaddress;
      printf("************** Mem_Init was a success totalsize %i. startaddress is %p. and final address is %p ***************\r\n\r\n",totalsize,startaddress,finaladdress);

   }
   close(fd);
   return 0;
}

//inserts a memory block into the linked list
//insert in ordered manner aka based on address
//ptr = address of new block (already confirmed enough space at address)
int insert(struct header* ptr) {
   printf("Starting insert at %p... ",ptr);
   struct header *t,*tn;
   t = p_head;
   if (ptr <= t) { //space below header, insert as new head
      printf("inserting node as new head... ");
      if (numblocks < 1) {
         printf(" its the only block!\r\n");
         ptr->next = NULL;
      } else {
         ptr->next = p_head; //set current pointer to correct address
      }
      p_head = ptr;
      return 0;
   } else {
      while (t != NULL) {
         tn = t->next; 
         if ((ptr > t)&&(tn == NULL)) { //inserting after current node as last node
            t->next = ptr;
            ptr->next = NULL;
            printf("inserted as last block after node %p\r\n",t);
            return 0;
         }
         if ((ptr > t)&&(ptr < tn)) { //inserting in between t and tn
            ptr->next = tn;
            t->next = ptr;
            printf("inserted as block after node %p and before node %p\r\n",t,tn);
            return 0;
         }
         t = tn;
      }
   }
   printf("Failed to find a place to insert the new block! FAILED\r\n");
   return -1; 
}

//removes a memory block from the linked list
//TODO add invalidate (aka change key when you remove)
int removeBlock(struct header* ptr) {
   printf("Starting remove method - removing at %p\r\n",ptr);
   struct header *temp,*prev,*next;
   temp = p_head;
   prev = NULL;
   while (temp != NULL) {
      next = temp->next; 
      if (temp == ptr) { //found block to remove
         if (prev == NULL) { //remove item as head
            p_head->key = 'l'; //invalidate key
            p_head = p_head->next;
            return 0;
         } else {
            prev->next = next;
            temp->key = 'l'; 
            temp->next = NULL;
            return 0;
         }
      }
      prev = temp;
      temp = temp->next;
   } 
   return -1;
}
/*
   if (p_head == ptr) { //removing head of list
      p_head = ntemp->next; 
      printf("Removed head of the list, new head at %p...",p_head);
      return 0; 
   } else {
      temp = ntemp->next; //starting at block past head
      prev = ntemp;
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
*/

//Finds smallest open block of memory that fits the size and returns the address
struct header* findsmallest(int size) {
   printf("BESTFIT... ");
   int smallest = totalsize; //set smallest to max
   struct header* smallestptr = NULL;
   struct header *curr = p_head; 
   struct header *tn;
   void *taddr;

   //Look for space above head
   int freespace = (void*)p_head - startaddress;
   if (freespace > size) {
      smallest = freespace;   
      smallestptr = (struct header *) startaddress;
      printf("found space above head: freespace = %i (p_head %p - startaddress %p)!\r\n",freespace,p_head,startaddress);
   }
   printf("searching...\r\n");
   while (curr != NULL) {
      tn = curr->next;
      taddr = (void *)curr + (curr->size);
      if (tn == NULL) { //found last block
         freespace = (finaladdress - taddr) ;
        // printf("------>freespace (%i) below last block (addr %p)...",freespace,curr);
         if ((freespace > size)&&(freespace < smallest)) {
            smallest = freespace;
            smallestptr = (struct header *)taddr; 
            printf(" found smallest spot at %p. ",taddr);
         }
      } else { //look in between blocks for space
         freespace = ((void*)tn - taddr);
        // printf("------>freespace (%i) inbetween block %p and %p...",freespace,curr,tn);
         if ((freespace > size)&&(freespace < smallest)) {
            smallest = freespace;
            smallestptr = (struct header *) taddr; 
            printf(" found smallest spot at %p. ",taddr);
         }
      }
      //printf("done checking block(%p).\r\n",curr);
      curr = tn; 
   }
   printf("done searching through linked list!\r\n");
   printf("Ending BESTFIT returning -------> %p!\r\n",smallestptr);
   return smallestptr;
}

void *Mem_Alloc(int size) {
   int fsize = size;
   struct header *addr;
   if (size <= 0) {
      perror("Mem_Alloc size equal to or less then zero!");
      exit(1);
   } else {
      printf("Called Mem_Alloc with size %i... ",size);
   }
   //change size to be 8-byte aligned
   int modval = size % 8;
   if (modval != 0) {
      int diff = 8 - modval;  
      fsize = size + diff;
      printf("size not 8-byte aligned changing to %i!\r\n",fsize);
   }
   //if no blocks implemented start new mem block at startaddress
   if (p_head == NULL) {
      printf("P_HEAD == NULL ------ Writing to head - at startaddress! (size = %i)\r\n",fsize);
      addr = (struct header *)startaddress;
      p_head = addr;
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
   printf("------------------------> wrote new block to address %p w/ size %i <-------------------------------\r\n",addr,addr->size);
   int check = insert(addr); //inserts and set next variable in header
   if (check == 0) {
      printf("Insert call was a success!\r\n");
      numblocks++;
   } else {
      printf("Insert call was a failure!FAILED !\r\n");
   }
   //allocate memory (save header) and return
   printf("Finishing call to mem_alloc - allocated block size %i to address %p!\r\n\r\n",addr->size,addr);
   return addr;
}

int Mem_Free(void *ptr) {
   struct header *rm = ptr;
   printf("Called MEM_FREE numblocks = %i\r\n",numblocks);
   if (ptr == NULL) {
      return -1;
   }
   if (numblocks <= 0) {
      printf("No blocks available to free!\r\n");
      return -1;
   }
   printf("trying to remove block with key %c, size %i!\r\n",rm->key,rm->size);
   if (rm->key != KEY) {
      printf("Found invalid key! Failed call to mem_free!\r\n");
      m_error = E_BAD_POINTER;
      return -1; 
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
   printf("--------------MEMORY BLOCK OF SIZE %i---------@%p-----\r\n",size,ptr);
   printf("--------------END OF MEMORY BLOCK------------------\r\n\r\n");
}

void printFreeBlock(int size, int *addr) {
   printf("--------------FREE BLOCK OF SIZE %i------@%p--------\r\n",size,addr);
   printf("--------------END OF FREE BLOCK------------------\r\n\r\n");
}

void Mem_Dump() {
   printf("Calling Mem_Dump().................................\r\n\r\n");
   printf("--------startaddress %p ------------------- finaladdress %p ----------- \r\n\r\n",startaddress,finaladdress);
   struct header *temp;  
   struct header *ntemp;
   ntemp = p_head;
   int gap;
   if (totalsize <= 0) printf("Memory has not been initialized! Failed!\r\n");
   if (numblocks <= 0) printf("No memory has been mapped, full memory of size %i available.\r\n",totalsize);
   if (numblocks > 0) { //Navigate through linked list of mem and print each section (including free sections)
      if (((void*)p_head - startaddress) > 0) {
         printf("GAP ON TOP OF MEM STACK - size = %i\r\n",((void*)p_head - startaddress));
      }
      printf("\r\n HEAD of list: size %i @ %p! ------------------------------>\r\n",ntemp->size,p_head);
      printf("--------------END OF MEMORY BLOCK------------------\r\n\r\n");
      temp = ntemp->next;
      while ((temp->next != NULL)&&(temp->key == KEY)) {
         printMemBlock(temp->size,(void*)temp);
         temp = temp->next;
      }
      //printf("Last Block has address %p with size %i and next %p (%i)\r\n\r\n",temp,temp->size,temp->next,(int*)temp->next);
      printMemBlock(temp->size,(void*)temp);
      gap = (finaladdress - ((void*)temp + (temp->size)));
      printf("LAST GAP IS size %i!!!\r\n",gap);
   }
}


