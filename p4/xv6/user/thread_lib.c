#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"

/*
 * 0 indicates lock is available, 1 means it is held
 */

lock_t lock;

int thread_create(void (*start_routine) (void *), void *arg) {
   void *stack;
  //call malloc to create a new user stack
  stack = malloc(sizeof(4096)); //PGSIZE = 4096
  int result = clone(start_routine, arg, stack);
  
  if (result == 0) { // process is now parent thread 
      printf(0,"This is parent thread\r\n");
  } else { //process is child thread
      printf(0,"This is child thread\r\n");
  }
  return result;
}

int thread_join() {
  void *childstack;
  int result = join(&childstack);
  free(childstack); 
  return result;
}

int lock_init(lock_t* l) {
  l->flag = 0;
  return 0;
}

int lock_acquire(lock_t* l) {
  //TODO disable interrupts?
  while (xchg((volatile uint *)&l->flag, 1) == 1) ; //do nothing
  return 0;
}

int lock_release(lock_t* l) {
  l->flag = 0;
  return 0;
}

