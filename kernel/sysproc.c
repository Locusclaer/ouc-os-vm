#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
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
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
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

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
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

// Print the current process's page table.
uint64
sys_vmprint(void)
{
  struct proc *p = myproc();

  printf("current process: pid=%d name=%s\n",
         p->pid, p->name);

  vmprint(p->pagetable);

  return 0;
}

// TODO: print pte\pa pages\offset\PA for the given virtual address.
// Print the mapping of a virtual address in the current process.
uint64
sys_vmlookup(void)
{
  uint64 va;
  argaddr(0, &va);

  struct proc *p = myproc();

  // walk() panics for addresses outside its supported range.
  if(va >= MAXVA){
    printf("VA %p: out of range\n", (void *)va);
    return -1;
  }

  // Query only: do not allocate missing page tables.
  pte_t *pte = walk(p->pagetable, va, 0);

  if(pte == 0 || (*pte & PTE_V) == 0){
    printf("VA %p: not mapped\n", (void *)va);
    return -1;
  }

  uint64 pa_page = PTE2PA(*pte);
  uint64 offset = va & (PGSIZE - 1);
  uint64 pa = pa_page + offset;

  printf("VA           : %p\n", (void *)va);
  printf("PTE          : %p\n", (void *)*pte);
  printf("PA page      : %p\n", (void *)pa_page);
  printf("offset       : %p\n", (void *)offset);
  printf("PA           : %p\n", (void *)pa);

  return 0;
}