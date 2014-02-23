// Create a zombie process that 
// must be reparented at exit.

#include "types.h"
#include "stat.h"
#include "user.h"

#include "../kernel/pstat.h"
int
main(void)
{
//Adde
struct pstat ptr;
//Testing tickets syscall
/*
if (settickets(2) == 0) {
   printf(0,"Set tickets success!\r\n");
} else {
   printf(0,"Set tickets success!\r\n");
}
*/
   
//Testing getpinfo
// getpinfo(ptr);
int inval = getpinfo(&ptr);

if (inval == 0) {
   int i;
   int max = 64;
   for(i=0;i<max;i++) {
      if (i < 3) printf(0,"Special val %d = %d\r\n",i,ptr.lticks[i]);
         printf(0,"Proc %d (pid=%d) is in use",i,ptr.pid[i]);
         printf(0,". It has %d hticks and %d lticks.\r\n"
                  ,ptr.hticks[i],ptr.lticks[i]);
   }
   printf(0,"Print out finished \r\n");
} else {
   printf(0,"Failed to call getpinfo!! \r\n");
   printf(0,"fail val= %d  \r\n",inval);
}


  if(fork() > 0)
    sleep(5);  // Let child exit before parent.

  exit();
}
