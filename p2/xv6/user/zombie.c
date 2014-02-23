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
      if ((ptr.inuse[i] == 0)&&(i < 10)) {
         printf(0,"Proc %d not in use pid=%d, hticks = %d, lticks = %d\r\n"
         ,i,ptr.pid[i],ptr.hticks[i],ptr.lticks[i]);
      }
      if(ptr.inuse[i] == 1) {
         printf(0,"Proc %d running!!! pid=%d, hticks = %d, lticks = %d\r\n"
            ,i,ptr.pid[i],ptr.hticks[i],ptr.lticks[i]);
      } else {
         //printf(0,"Proc %d not in use pid=%d, hticks = %d, lticks = %d\r\n"
          //  ,i,ptr.pid[i],ptr.hticks[i],ptr.lticks[i]);
      }
      if (i == 60) printf(0,"lotto = %d\r\n",ptr.lticks[i]);
      if (i == 61) printf(0,"qlevel = %d\r\n",ptr.lticks[i]);
      if (i == 62) printf(0,"tix = %d\r\n",ptr.lticks[i]);
      if (i == 63) printf(0,"lo_tix= %d\r\n",ptr.lticks[i]);
      if (i == 59) printf(0,"hi_tix = %d\r\n",ptr.lticks[i]);
      if (i == 58) printf(0,"Seen ticket? = %d\r\n",ptr.lticks[i]);
   }
   printf(0,"\r\n");
   printf(0,"Print out finished \r\n");
} else {
   printf(0,"Failed to call getpinfo!! \r\n");
   printf(0,"fail val= %d  \r\n",inval);
}


  if(fork() > 0)
    sleep(5);  // Let child exit before parent.

  exit();
}
