// Create a zombie process that 
// must be reparented at exit.

#include "types.h"
#include "stat.h"
#include "user.h"

#include "../kernel/pstat.h"
int
main(void)
{
//Added
struct pstat *ptr = 0;
struct pstat local;
//Testing tickets syscall
if (settickets(2) == 0) {
   printf(0,"Set tickets success!\r\n");
} else {
   printf(0,"Set tickets success!\r\n");
}
   
//Testing getpinfo
int inval = getpinfo(ptr);
if (inval == 0) {
   if(ptr == 0) printf(0,"PTR NOT SET AHHH\r\n");
   local = *ptr;
   int i;
   int max = 150;
   for(i=0;i<max;i++) {
      if (local.inuse[i] == 0) {
         printf(0,"Proc %d (pid=%d) is not in use \r\n",i,local.pid[i]);
      } else {
         printf(0,"Proc %d (pid=%d) is in use",i,local.pid[i]);
         printf(0,". It has %d hticks and %d lticks.\r\n"
                  ,local.hticks[i],local.lticks[i]);
      }
   }
   printf(0,"Print out finished \r\n");
} else {
   printf(0,"Failed to call getpinfo!! \r\n");
   printf(0,"But testing value does = %d  \r\n",inval);
}

/*
  if(fork() > 0)
    sleep(5);  // Let child exit before parent.
*/
  exit();
}
