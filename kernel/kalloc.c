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
} kmem[NCPU];

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
  for (int i = 0; i < NCPU; i++)
    initlock(&kmem[i].lock, "kmem");
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
  int _cpuid = ((uint64)pa / PGSIZE) % NCPU;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[_cpuid].lock);
  r->next = kmem[_cpuid].freelist;
  kmem[_cpuid].freelist = r;
  pageref[((uint64)r - KERNBASE)/PGSIZE] = 1;
  release(&kmem[_cpuid].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int _cpuid = cpuid();

  acquire(&kmem[_cpuid].lock);
  r = kmem[_cpuid].freelist;
  if(r)
    kmem[_cpuid].freelist = r->next;
  release(&kmem[_cpuid].lock);

  // steal mem from other cpu's mem
  if (!r) {
    for (int i = 0; i < NCPU; i++) {
      if (i == _cpuid)
        continue;

      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      if(r) {
        kmem[i].freelist = r->next;
        release(&kmem[i].lock);
        break;
      }
      release(&kmem[i].lock);
    }
  }

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    pageref[((uint64)r - KERNBASE)/PGSIZE] = 1;
  }

  return (void*)r;
}

int
get_freemem_size(void)
{
  struct run *r;
  int num = 0;

  for (int i = 0; i < NCPU; i++) {
    for (r = kmem[i].freelist; r; r = r->next)
      num++;
  }

  return num * PGSIZE;
}
