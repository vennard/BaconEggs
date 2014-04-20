#include "types.h"
#include "stat.h"
#include "user.h"

lock_t lock;

int thread_create(void (*start_routine) (void *), void *arg) {
  return 4;
}

int lock_init(lock_t* l) {
  return 6;
}

int lock_acquire(lock_t* l) {
  return 7;
}

int lock_release(lock_t* l) {
  return 8;
}

