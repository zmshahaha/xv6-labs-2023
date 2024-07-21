#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  backtrace();

  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 addr;
  int num;
  uint64 bitmap;
  pte_t *pte;
  uint32 tmp = 0;
  pagetable_t pagetable = myproc()->pagetable;

  argaddr(0, &addr);
  argint(1, &num);
  argaddr(2, &bitmap);

  if((num > 32) || (num <= 0))
    return -1;

  for(int i = 0; i < num; i++){
    pte = walk(pagetable, addr + i*PGSIZE, 0);
    if(*pte & PTE_A){
      tmp |= (1<<i);
      *pte &= (~PTE_A);
    }
  }

  copyout(pagetable, bitmap, (char *)&tmp, 4);
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  struct proc *curr = myproc();

  argint(0, &curr->alarm_interval);
  argaddr(1, &curr->alarm_fn);
  curr->next_alarm = ticks + curr->alarm_interval;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  memmove(p->trapframe, &p->signal_trapframe, sizeof(p->signal_trapframe));
  p->next_alarm = ticks + p->alarm_interval;
  p->sighandling = 0;
  return p->trapframe->a0;
}
