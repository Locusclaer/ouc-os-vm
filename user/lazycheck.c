#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"
#include "kernel/fcntl.h"

static void
fail(char *message)
{
  printf("FAIL: %s\n", message);
  exit(1);
}

int
main(void)
{
  // Extra space allows alignment even if the old break is unaligned.
  char *base = sbrklazy(4 * PGSIZE);
  if(base == SBRK_ERROR)
    fail("sbrklazy");

  volatile char *p =
    (volatile char *)PGROUNDUP((uint64)base);

  printf("[1] lazy allocation and zero initialization\n");

  // Querying must not allocate the page.
  if(vmlookup((uint64)p) != -1)
    fail("page was allocated before first access");

  if(p[0] != 0)
    fail("new page is not zero");

  p[0] = 11;
  if(p[0] != 11)
    fail("first page read/write");

  if(vmlookup((uint64)p) != 0)
    fail("page missing after first access");

  printf("PASS\n");

  printf("[2] multiple pages\n");
  p[2 * PGSIZE] = 22;
  if(p[0] != 11 || p[2 * PGSIZE] != 22)
    fail("multiple pages");
  printf("PASS\n");

  printf("[3] accesses on both sides of a page boundary\n");
  p[PGSIZE - 1] = 33;
  p[PGSIZE] = 44;
  if(p[PGSIZE - 1] != 33 || p[PGSIZE] != 44)
    fail("page boundary");
  printf("PASS\n");

  printf("[4] fork with an untouched page\n");

  // p[3 * PGSIZE] has not been accessed by the previous tests.
  if(vmlookup((uint64)(p + 3 * PGSIZE)) != -1)
    fail("expected an unmapped page before fork");

  int forkchild = fork();
  if(forkchild < 0)
    fail("fork with holes");

  if(forkchild == 0){
    if(p[0] != 11)
      fail("child did not inherit existing data");

    if(p[3 * PGSIZE] != 0)
      fail("child lazy page is not zero");

    p[0] = 66;
    p[3 * PGSIZE] = 77;
    exit(0);
  }

  int forkstatus = 0;
  if(wait(&forkstatus) != forkchild || forkstatus != 0)
    fail("child failed");

  if(p[0] != 11)
    fail("child changed parent's existing page");

  if(vmlookup((uint64)(p + 3 * PGSIZE)) != -1)
    fail("child allocated a page in parent's page table");

  if(p[3 * PGSIZE] != 0)
    fail("parent lazy page is not independent");

  printf("PASS\n");

  printf("[5] kernel copyin and copyout\n");

  char *iobase = sbrklazy(4 * PGSIZE);
  if(iobase == SBRK_ERROR)
    fail("reserve I/O buffers");

  char *q = (char *)PGROUNDUP((uint64)iobase);

  int fds[2];
  if(pipe(fds) < 0)
    fail("pipe");

  // The kernel must read an untouched lazy page through copyin().
  if(vmlookup((uint64)q) != -1)
    fail("copyin buffer was already mapped");

  char received = 99;
  if(write(fds[1], q, 1) != 1)
    fail("copyin from lazy buffer");
  if(read(fds[0], &received, 1) != 1 || received != 0)
    fail("copyin data");

  // The kernel must write an untouched lazy page through copyout().
  if(vmlookup((uint64)(q + PGSIZE)) != -1)
    fail("copyout buffer was already mapped");

  char sent = 'K';
  if(write(fds[1], &sent, 1) != 1)
    fail("write to pipe");
  if(read(fds[0], q + PGSIZE, 1) != 1)
    fail("copyout to lazy buffer");
  if(q[PGSIZE] != 'K')
    fail("copyout data");

  close(fds[0]);
  close(fds[1]);
  printf("PASS\n");

  printf("[6] shrink memory containing holes\n");

  for(int round = 0; round < 8; round++){
    char *oldbreak = sbrk(0);
    char *extra = sbrklazy(4 * PGSIZE);
    if(extra == SBRK_ERROR)
      fail("reserve memory for shrinking");

    volatile char *area =
      (volatile char *)PGROUNDUP((uint64)extra);

    // Allocate some pages, leaving other pages untouched.
    area[0] = 1;
    area[2 * PGSIZE] = 2;

    if(sbrk(-4 * PGSIZE) == SBRK_ERROR)
      fail("shrink");
    if(sbrk(0) != oldbreak)
      fail("break was not restored");

    if(vmlookup((uint64)area) != -1)
      fail("mapping survived shrinking");
  }

  printf("PASS\n");

  printf("[7] invalid address\n");
  int child = fork();
  if(child < 0)
    fail("fork");

  if(child == 0){
    volatile char *bad =
      (volatile char *)PGROUNDUP((uint64)sbrk(0));
    *bad = 1;
    exit(0);
  }

  int status = 0;
  if(wait(&status) != child || status != -1)
    fail("invalid access did not kill child");

  printf("PASS\n");
  printf("LAZYCHECK PASSED\n");
  exit(0);
}