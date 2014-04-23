#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"

int ppid;
volatile int global = 1;
#define PGSIZE (4096)

void worker(void *arg_ptr);

int main(void) {
   printf(0,"Starting user testing... ");

   ppid = getpid();
   void *stack = malloc(PGSIZE*2);
   printf(0,"allocated stack! ");
   if((uint)stack % PGSIZE)
     stack = stack + (4096 - (uint)stack % PGSIZE);
   printf(0,"page aligned request... now calling clone\r\n ");
   printf(0,"fcn = %p, 0, stack = %p!!!\r\n",worker,stack);
   int clone_pid = clone(worker, 0, stack);
   printf(0, "clone_pid = %d\r\n",clone_pid);
   while(global != 5);
   printf(0, "TEST PASSED\n");

   exit();
}

void
worker(void *arg_ptr) {
   //assert(global == 1);
   global = 5;
   exit();
}


/*
   printf(0,"Trying to call clone... ");
   int a = clone();
   if (a == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }
   printf(0,"Trying to call join... ");
   int b = join();
   if (b == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }
   ptr = &a;

   printf(0,"Trying to call thread_create... ");
   int c = thread_create(ptr, ptr);
   if (c == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }

   printf(0,"Trying to call thread_join... ");
   int g = thread_join();
   if (g == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }

   printf(0,"Trying to call lock_init... ");
   int d = lock_init(&lock);
   if (d == 0)  {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }

   printf(0,"Trying to call lock_acquire... ");
   int e = lock_acquire(&lock);
   if (e == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }

   printf(0,"Trying to call lock_release... ");
   int f = lock_release(&lock);
   if (f == 0) {
      printf(0,"success!\r\n");
   } else {
      printf(0,"failed!\r\n");
   }
   exit();
}
*/
