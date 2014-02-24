#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

//Test Program to return info on scheduler
//used to create graph for p2 turnin
int main(void) {
   struct pstat pst;
   //assign tickets to process
   int numtickets = 3;
   if (settickets(numtickets) == -1) printf(0,"Error with settickets!\r\n");
   
   //run some large processes
   //user_tests();
   //time_test();
   
   //Get pstat info
   int i;
   int max = 64;
   int total_hticks = 0;
   int total_lticks = 0;
   int running_procs = 0;
   if (getpinfo(&pst) == 0) {
      for(i=0;i<max;i++) {
         if (pst.inuse[i] == 1) running_procs++; 
         if (pst.lticks[i] > 0) total_lticks+=pst.lticks[i];
         if (pst.hticks[i] > 0) total_hticks+=pst.hticks[i];
         //Debug print out for my sanity 
         printf(0,"%d: pid = %d, ",i,pst.pid[i]); 
         printf(0,"inuse = %d, ",pst.inuse[i]); 
         printf(0,"hticks = %d, ",pst.hticks[i]); 
         printf(0,"lticks = %d.\r\n",pst.lticks[i]); 
      }
      //Debug print out -- important info
      printf(0,"RESULTS:\r\n");
      printf(0,"This procs pid = %d which has %d tickets.\r\n",getpid(),numtickets);
      printf(0,"running processes = %d\r\n",running_procs);
      printf(0,"total hticks = %d\r\n",total_hticks);
      printf(0,"total lticks = %d\r\n\r\n",total_lticks);
      printf(0,"SUCCESS - completed without errors.\r\n");
      
   } else printf(0,"Error with getpinfo!\r\n");

   exit();
}

