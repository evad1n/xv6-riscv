#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

  if(argint(0, &pid) < 0)
    return -1;
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

// Recursively print page-table pages.
void
printwalk(pagetable_t pagetable, int depth)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    
    // If it is a valid page table
    if (pte & PTE_V)
    {
      if (depth == 0)
        printf("\n");

      // Show nested structure
      for (int i = 0; i < depth; i++)
      {
        if (i == depth - 1)
        {
          printf("|\n");
          for (int i = 0; i < depth-1; i++)
            printf("      ");
          printf("+---");
        }
        else
          printf("      ");
      }
      
      // Print info
      uint64 pa = PTE2PA(pte);
      printf("> Page %d: pte %p, pa %p, refs: %d, writable: %s\n", i, pte, pa, getref(pa), (pte & PTE_W) ? "True" : "False");

      // If pointing to a lower-level page table
      if (!(pte & (PTE_R|PTE_W|PTE_X)))
      {
        uint64 child = PTE2PA(pte);
        printwalk((pagetable_t)child, depth+1);
      }
    }
  }
}

// proc data in kernel/proc.h
uint64
sys_pages(void)
{
  pagetable_t pagetable = myproc()->pagetable;
  printf("\npage table %p\n", pagetable);
  printwalk(pagetable, 0);
  printf("\n");
  return 0;
}

uint64
sys_freepages(void)
{
  return num_free_pages();
}