#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"

/*
 * 0 indicates lock is available, 1 means it is held
 */

lock_t lock;
void *stack;

int thread_create(void (*start_routine) (void *), void *arg) {
  //call malloc to create a new user stack
  stack = malloc(4096*2); //PGSIZE = 4096
  printf(0, "IN THREAD_CREATE: malloc'd\r\n");
  int result = clone(start_routine, arg, stack);
  printf(0, "IN THREAD_CREATE: result of clone = %d\r\n",result);
  printf(0,"THREAD_CREATE: stack = %p\r\n",stack);
  return result;
}

int thread_join() {
  void *childstack;
  int result = join(&childstack);
  free(childstack); 
  printf(0,"THREAD_JOIN: stack = %p, and returned pid = %d\r\n",childstack,result);
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

