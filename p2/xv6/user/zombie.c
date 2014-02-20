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
int pid = getpinfo(ptr);
printf(0,"THIS IS IT PID = %d \r\n",pid);
  if(fork() > 0)
    sleep(5);  // Let child exit before parent.
  exit();
}
