#ifndef __include_h__
#define __include_h__
// Holds globals for linked list and structure for header

#define KEY 'z'

struct header{
   char key; //unique key for validating headers
   int size; //size of total block including header
   struct header *next;
};

#endif 


