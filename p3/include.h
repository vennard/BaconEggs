#ifndef __include_h__
#define __include_h__
// Holds globals for linked list and structure for header

#define KEY 'z'

extern int *startaddress; //holds address returned by mmap for mem_init
extern int totalsize; //holds total size of memory allocated in mem_init
extern struct header head; //head of linked list of memory
extern int numblocks; //holds number of memory blocks currently allocated

struct header{
   char key; //unique key for validating headers
   int size; //size of total block including header
   struct header *next;
};

#endif 


