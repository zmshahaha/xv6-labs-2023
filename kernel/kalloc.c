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
} kmem;

#define PAGE_NUM ((PHYSTOP) - (KERNBASE))/(PGSIZE)
static int pageref[PAGE_NUM];

void page_ref(uint64 pa) {
  int index = (pa-KERNBASE)/PGSIZE;
  if ((index < 0) || (index >= PAGE_NUM))
    panic("invalid pa");
  pageref[index] ++;
}

void page_unref(uint64 pa) {
  int index = (pa-KERNBASE)/PGSIZE;
  if ((index < 0) || (index >= PAGE_NUM))
    panic("invalid pa");
  pageref[index] --;
  if (pageref[index] == 0) {
    kfree((void *)pa);
  }
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  pageref[((uint64)r - KERNBASE)/PGSIZE] = 1;
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
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  pageref[((uint64)r - KERNBASE)/PGSIZE] = 1;
  return (void*)r;
}

int
get_freemem_size(void)
{
  struct run *r;
  int num = 0;
  for (r = kmem.freelist; r; r = r->next)
    num++;
  return num * PGSIZE;
}
