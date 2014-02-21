#include "types.h"
#include "stat.h"
#include "user.h"

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

//THIS METHOD USES CMWC Algorithm
int
main(void)
{
  //Testing time randomization 
  int t = uptime();
  printf(0,"Clks since boot: %d\n",t);
  //test random number
  //Using Complementary-Multiply-with-carry method
  init(33256246);
   //RUN TESTING TO SEE HOW RANDOMIZED IT IS
  uint a = rand();
  printf(0,"random number: %d\n",a);
  int j;
   int RUNTIME = 80000;
   int results[100];
   for(j=0;j<RUNTIME;j++) {
      int rnd = rand() % 100;
      results[rnd] = results[rnd] + 1;
   } 
   int k;
   printf(0,"Results after %d runs:\n",RUNTIME);
   for(k=0;k<100;k++) {
      printf(0,"%d - %d, ",k,results[k]);
   if ((k%10)==9) printf(0,"\n");
   }
   int avg,i; 
   avg = 0;
   for(i=0;i<100;i++) avg += results[i];
   avg = avg / 100; 
   printf(0,"\nAverage: %d",avg);
   
  exit();
}
