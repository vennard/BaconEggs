#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct pstat pst;
struct pstat *pptr;
int testing = 0;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
//ADDED BELOW TODO label
#define RND 0x3e425212

static uint x[4096];
static uint b;

int uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

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
   //t = (1234567 * x[i]) + b;
   t = (uptime() * x[i]) + b;
   b = t >> 16;
   x[i] = 0xfffffffe - t;
   return x[i];
}
//Generate random #
//Using Complementary-Multiply-with-carry method
int get_rand(void) { 
   init(33256246);
   return (int) rand();
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  int hi_tix = 0;
  int lo_tix = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // TODO Scheduler prep code 
    // Search through procs and count # in each queue 
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE) continue;
         /*
         proc = p;
         switchuvm(p);
         p->state = RUNNING;
         swtch(&cpu->scheduler, proc->context);
         switchkvm();
      //Run twice for LOW priority queue (second run)
      if (p->level == 2) {
         p->level = 1; //set back to normal low priority
         //execute process
         proc = p;
         switchuvm(p);
         p->state = RUNNING;
         swtch(&cpu->scheduler, proc->context);
         switchkvm();
      }
         */
      if (p->level == 0) {
      //else if(p->level == 0) {
        //Count the number of tickets assigned at high priority
        //Assign ticket if no ticket has been assigned
       if (p->tickets <= 0) {
           p->tickets = 1;
           hi_tix++;
        } else {
           hi_tix += p->tickets;
        }
      }
      else if(p->level == 1) {
        //Might want to check that it has a ticket - just to be sure
        lo_tix += p->tickets;
      }
    }
    release(&ptable.lock);

    //Generate & mod lottery ticket num
    //int rnd = get_rand(); //TODO put random number here
    int rnd = 140;
    int lotto = 0;
    int tix;
    if (hi_tix > 0) {
      lotto = rnd % hi_tix; 
      tix = hi_tix;
    } else if (lo_tix > 0) {
      lotto = rnd % lo_tix;
      tix = lo_tix;
    } else {
      lotto = -1; //fail safe
      tix = 0;
    }

    //Generate new tickets for all proc in executing level
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
       if(p->state != RUNNABLE) continue;
       if((hi_tix > 0)&&(p->tickets > 0)) {
         //assign tickets   
         int j;
         for(j=0;j<p->tickets;j++) {
            p->ticket[j] = hi_tix;
            hi_tix--;
         }
       } else if((lo_tix > 0)&&(p->tickets > 0)) {
          //assign tickets   
          int j;
          for(j=0;j<p->tickets;j++) {
             p->ticket[j] = lo_tix;
             lo_tix--;
          }
       }     
    }
    release(&ptable.lock);
/*
         proc = p;
         switchuvm(p);
         p->state = RUNNING;
         swtch(&cpu->scheduler, proc->context);
         switchkvm();
      proc = 0;
  }
}
*/

//TODO TEsting REMOVE
      struct pstat tp;
      tp.hticks[0] = 3555; //set pids
      tp.hticks[1] = 3555; //set pids
      tp.hticks[2] = 3555; //set pids
      tp.hticks[3] = 3425; //set pids
      tp.inuse[0] = 1;
      tp.inuse[1] = 1;
      tp.inuse[3] = 1;
    //2-Level Lottery Scheduler
    //Loop over process table to identify lottery winner
    int index = 0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      pst.pid[index] = p->pid; //set pids
      //tp.pid[index] = p->pid;
      if(p->state != RUNNABLE) {
         pst.inuse[index] = 0; //set to not in use
         continue;
      }
         //p->pstat_t = pst; 
         testing = pst.pid[index];
         p->pstat_t = tp; //TODO
         //pptr = p->pstat_t;
         proc = p;
         switchuvm(p);
         p->state = RUNNING;
         swtch(&cpu->scheduler, proc->context);
         switchkvm();
      proc = 0;
    }
    release(&ptable.lock);
  }
}

    /*
      //running hi priority queue
      if (hi_tix > 0) {
         if (p->level == 0) {
            int i;
            //maybe add a max check for tickets (255)
            for (i=0;i<p->tickets;p++) {
               //Check if process has matching ticket
               if(p->ticket[i] == lotto) {
                  p->level = 1; //move to low priority queue
                  pst.hticks[index]++;
                  pst.inuse[index] = 1; 
                  //execute proc
                  proc = p;
                  switchuvm(p);
                  p->state = RUNNING;
                  swtch(&cpu->scheduler, proc->context);
                  switchkvm();
               }
            }
         }
      //Otherwise running low priority queue
      } else if (lo_tix > 0) {
         if (p->level == 1) {
            int i;
            //maybe add a max check for tickets (255)
            for (i=0;i<p->tickets;p++) {
               if(p->ticket[i] == lotto) {
                  p->level = 2;  //setup proc to run twice 
                  pst.inuse[index] = 1; 
                  pst.inuse[index] = 1; 
                  pst.lticks[index] += 2;
                  // Switch to chosen process.  It is the process's job
                  // to release ptable.lock and then reacquire it
                  // before jumping back to us.
                  proc = p;
                  switchuvm(p);
                  p->state = RUNNING;
                  swtch(&cpu->scheduler, proc->context);
                  switchkvm();
               }
            }
         }
      }

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      index++;  
      proc = 0; 
    } 
    release(&ptable.lock);
  }
}
*/

   

//TODO END OF ADDED CODE
 
// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


