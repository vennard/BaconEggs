#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"

int ppid;
volatile int global = 1;
volatile int arg = 55;
#define PGSIZE (4096)

void worker(void *arg_ptr);

int main(void) {
   printf(0,"Starting user testing... ");
   ppid = getpid();
   //void *stack = malloc(PGSIZE*2);
   printf(0,"allocated stack! ");
/*

   if((uint)stack % PGSIZE)
     stack = stack + (4096 - (uint)stack % PGSIZE);
   printf(0,"Calling clone with fcn = %p, 0, stack = %p!!!\r\n",worker,stack);

   int clone_pid = clone(worker, (void*)&arg, stack);
   printf(0, "clone_pid = %d\r\n",clone_pid);

   void *join_stack;
   printf(0,"SHOULD SAVE TO &JOIN_STACK = %p\r\n",&join_stack);
   int join_pid = join(&join_stack);
   printf(0,"JOIN results: returned pid- %d, join_stack- %p!\r\n",join_pid,join_stack);
*/
   int i;
   for (i = 0; i < 2;i++) {
   printf(0,"\r\nLOOP: %d!!!\r\n",i);
   int thread_pid = thread_create(worker,0);
   printf(0,"Created THREADS PID = %d\r\n",thread_pid);
   int join_pid = thread_join();
   printf(0,"join resulted in join_pid=%d\r\n",join_pid);
   sleep(10);
   //while(global != 5) ;
   if (thread_pid != join_pid) {
      printf(0, "TEST FAILED \r\n");
   } else {
      printf(0, "TEST PASSED \r\n");
   } 
    } 
   exit();
}

void
worker(void *arg_ptr) {
   int tmp = *(int*)arg_ptr;
   printf(0,"WORKER THREAD: ARG=%d (from pointer %p)!\r\n",tmp,arg_ptr);
   global = tmp;
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
