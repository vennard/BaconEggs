#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

//Randomization code
#define RND 0x3e425212

static uint x[4096];
static uint b;
void init(uint y) {
   int i;
   x[0] = y;
   x[1] = y + RND;
   x[2] = y + RND + RND;
   for(i=3;i<4096;i++) x[i] = x[i-3] ^ x[i-2] ^ RND ^ i;
}
uint rand(void) {
   static uint i = 4095;
   uint t;
   i = (i+1) & 4095;
   //seed is uptime clock ticks since boot
   t = (uptime() * x[i]) + b;
   b = t >> 16;
   x[i] = 0xfffffffe - t;
   return x[i];
}

//Test Program to return info on scheduler
//used to create graph for p2 turnin
int main(void) {
   struct pstat pst;
   int max = 64;
   int time_steps = 10;
   int xvals[time_steps];
   int yvals[time_steps];
   //fork to create new process
   if (fork() > 0) {
      int my_pid = getpid();
      //is parent
      //assign tickets to process
      int numtickets = 5;
      if (settickets(numtickets) == -1) printf(0,"Error with settickets!\r\n");

      // Time step through collecting data points
      int j;
      for (j=0;j<time_steps;j++) {
         //get data
         if (getpinfo(&pst) != 0) printf(0,"Error calling getpinfo!!!\r\n");
         //record x data
         int t_timeslices = 0;
         int t_myslices = 0;
         int t_procs = 0;
         int i;
         for(i=0;i<max;i++) {
            if (pst.inuse[i] == 1) t_procs++; 
            if (pst.lticks[i] > 0) t_timeslices+=pst.lticks[i];
            if (pst.pid[i] == my_pid) t_myslices = pst.lticks[i];
         }
         printf(0,"ENTRY %d: \r\n",j);
         printf(0,"my_tickets / total tickets: x = %d y = %d\r\n",numtickets,t_procs + numtickets -1);
         printf(0,"my_slices / total slices: x = %d y = %d\r\n",t_myslices,t_timeslices);
         printf(0," \r\n");
         int x = numtickets / (t_procs + numtickets - 1);
         int y = t_myslices / t_timeslices;
         //save off   
         xvals[j] = x;
         yvals[j] = y;
         //wait 
         sleep(100);
      }
      //wait for child to do some stuff
      sleep(200);

   //run some large processes
   //user_tests();
   //time_test();
   
   //Get final pstat info 
   int i;
   int total_hticks = 0;
   int total_lticks = 0;
   int running_procs = 0;
   int my_hticks = 0;
   int my_lticks = 0;
   if (getpinfo(&pst) == 0) {
      for(i=0;i<max;i++) {
         if (pst.inuse[i] == 1) running_procs++; 
         if (pst.lticks[i] > 0) total_lticks+=pst.lticks[i];
         if (pst.hticks[i] > 0) total_hticks+=pst.hticks[i];
         if (pst.pid[i] == my_pid) {
            my_hticks = pst.hticks[i];
            my_lticks = pst.lticks[i];
         }
         //Debug print out for my sanity 
         /*
         printf(0,"%d: pid = %d, ",i,pst.pid[i]); 
         printf(0,"inuse = %d, ",pst.inuse[i]); 
         printf(0,"hticks = %d, ",pst.hticks[i]); 
         printf(0,"lticks = %d.\r\n",pst.lticks[i]); 
         */
      }
      //Debug print out -- important info
      printf(0,"RESULTS OF FINAL PSTAT RUN:\r\n");
      printf(0,"This procs pid = %d which has %d tickets.\r\n",my_pid,numtickets);
      printf(0,"total tickets assigned = %d\r\n",running_procs+(numtickets-1));
      printf(0,"total hticks = %d vs. this proc hticks = %d\r\n",total_hticks,my_hticks);
      printf(0,"total lticks = %d vs. this proc lticks = %d\r\n\r\n",total_lticks,my_lticks);
      printf(0,"SUCCESS - completed without errors.\r\n");
      printf(0,"\r\n\r\nNow printing out graph data points\r\n");
      /*
      int l;
      for (l=0;l<time_steps;l++) {
         printf(0,"Step %d: x=%d   y=%d\r\n",l,xvals[l],yvals[l]); 
      }
      */
      printf(0,"Finished - completed without errors.\r\n");
   } else printf(0,"Error with getpinfo!\r\n");

   } else {
      //is child
      //make child do lots of stuff to give good data for graph
      init(33256246);
      int j;
      int RUNTIME = 10000000;
      int results[100];
      printf(0,"Child generating %d random numbers...\r\n",RUNTIME);
      for(j=0;j<RUNTIME;j++) {
         int rnd = rand() % 100;
         results[rnd] = results[rnd] + 1;
      } 
      int avg,i; 
      avg = 0;
      for(i=0;i<100;i++) avg += results[i];
         avg = avg / 100; 
      if (avg != RUNTIME / 100) {
         printf(0,"Childs random average incorrect!!! it was %d\r\n",avg);
      } else {
         //printf(0,"Childs random average is good!!! it was %d\r\n",avg);
      }
   } 

   exit();
}

