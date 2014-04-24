#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/*
 * Creates new kernel thread
 * new process shares address space but has its own stack
 * fcn - new thread starts executing at this address 
 * arg - passed given argument
 * stack - new user stack address
 */
int sys_clone(void)
{
  void *fcn, *arg, *stack;   
  fcn = 0; 
  arg = 0;
  stack = 0;
  if (argptr(0, (char**)&fcn, sizeof(fcn)) < 0) return -1; 
  if (argptr(1, (char**)&arg, sizeof(arg)) < 0) return -1; 
  if (argptr(2, (char**)&stack, sizeof(stack)) < 0) return -1; 
  cprintf("IN SYS_CLONE: fcn = %p, arg = %d, stack = %p\r\n",fcn,0,stack); 
  int result = clone(fcn, arg, stack);
  return result;
}

int sys_join(void)
{
  void *stack; //return &stack (void **)
  stack = 0;
  if (argptr(0, (void *)stack, sizeof(stack)) < 0) return -1;
  int result = join(&stack);
  return result;
}
