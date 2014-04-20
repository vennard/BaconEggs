#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"

/*
 * 0 indicates lock is available, 1 means it is held
 */

lock_t lock;

int thread_create(void (*start_routine) (void *), void *arg) {
  //call malloc to create a new user stack
  //call clone() to create the child thread 
  return 0;
}

int thread_join() {
  return 0;
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

