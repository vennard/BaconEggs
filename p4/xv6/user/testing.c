#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"


int main(void) {
   printf(0,"Starting user testing...\r\n");
   void *ptr;

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
