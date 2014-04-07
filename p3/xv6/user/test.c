#include "types.h"
#include "user.h"
#include "stat.h"
#include "../kernel/mmu.h"

int 
main(void){
  int* ptr = (int*) PGSIZE;
  printf(0, "Contents of memory location %p: %d\n", ptr, *ptr);
  exit();
}
