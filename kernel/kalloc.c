// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint free_pages; // Tracks the number of free pages in the kernel
  char pg_refcount[(PHYSTOP - KERNBASE) >> PGSHIFT]; // page reference counts
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.free_pages = 0; // No free pages at start
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    kmem.pg_refcount[PA2RIDX(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run*)pa;

  acquire(&kmem.lock);

  // Char underflow wraps to 255, so we don't want that...
  if(kmem.pg_refcount[PA2RIDX(pa)] > 0)
    kmem.pg_refcount[PA2RIDX(pa)]--;

  /* Only free if there are no more references to the page */
  if (kmem.pg_refcount[PA2RIDX(pa)] == 0)
  {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r->next = kmem.freelist;
    kmem.freelist = r;

    kmem.free_pages++; // Increment num free pages
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    kmem.freelist = r->next;
    // Decrement free pages
    kmem.free_pages--;
    // Set reference count to 1
    kmem.pg_refcount[PA2RIDX(r)] = 1;  
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  return (void*)r;
}

/* Returns the total number of free pages in the kernel */
int num_free_pages(void)
{
  acquire(&kmem.lock);
  int num = kmem.free_pages;
  release(&kmem.lock);
  return num;
}

/* Gets the reference counter for the page at the physical address pa */
int getref(uint64 pa)
{
  acquire(&kmem.lock);
  int count = kmem.pg_refcount[PA2RIDX(pa)];
  release(&kmem.lock);
  return count;
}

/* Increments the reference counter for the page at the physical address pa */
int incref(uint64 pa)
{
  acquire(&kmem.lock);
  int count = ++kmem.pg_refcount[PA2RIDX(pa)];  
  release(&kmem.lock);
  return count;
}

/* Decrements the reference counter for the page at the physical address pa */
int decref(uint64 pa)
{
  acquire(&kmem.lock);
  int count = --kmem.pg_refcount[PA2RIDX(pa)];  
  release(&kmem.lock);
  return count;
}