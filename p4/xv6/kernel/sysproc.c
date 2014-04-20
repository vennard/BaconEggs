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
  int i, pid;
  struct proc *np;
  void *fcn, *arg, *stack;   
  fcn = 0; 
  arg = 0;
  stack = 0;
  if (argptr(0, (void *)fcn, sizeof(fcn)) < 0) return -1; 
  if (argptr(1, (void *)arg, sizeof(arg)) < 0) return -1; 
  if (argptr(2, (void *)stack, sizeof(stack)) < 0) return -1; 
   
  np = proc; //TODO REMOVE THIS IS TOTALLY WRONG
  //acquire(&ptable.lock);
  //for(np = ptable.proc;np
  //release(&ptable.lock);
  //np->thread = 1; 
  //allocate process -- finds in table and sets state to EMBRYO
  //if ((np = allocproc()) == 0) return -1; 
  //acquire(&lock);
  //np->kstack = stack; TODO PUT BACK IN
  //TODO save arg on the stack
    
  
  //copy process state from p 
  //copyuvm - copies page table (address space)
  if ((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0) {
    kfree(np->kstack); //free page of mem at kstack
    np->kstack = 0; //set bottom of stack to 0
    np->state = UNUSED; 
    return -1;
  }
  np->sz = proc->sz; //copy size of process mem
  np->parent = proc; //set parent process
  *np->tf = *proc->tf; //set trapframe for syscalls
 
  //clear %eax so that fork returns 0 in the child
  np->tf->eax = 0;
 
  for (i = 0;i < NOFILE;i++) if (proc->ofile[i]) np->ofile[i] = filedup(proc->ofile[i]);

  np->cwd = idup(proc->cwd); //copy over current directory
  pid = np->pid;             
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

//TODO
int sys_join(void)
{
  void **stack;
  stack = 0;
  if (argptr(0, (void *)stack, sizeof(stack)) < 0) return -1;
  return 0;
}
