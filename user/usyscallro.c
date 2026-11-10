#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "user/user.h"

int
main(void)
{
  if(ugetpid() != getpid()){
    printf("FAIL: parent PID mismatch\n");
    exit(1);
  }

  int child = fork();
  if(child < 0){
    printf("FAIL: fork\n");
    exit(1);
  }

  if(child == 0){
    // The child must read its own PID, not its parent's PID.
    if(ugetpid() != getpid()){
      printf("FAIL: child PID mismatch\n");
      exit(2);
    }

    printf("child PID check passed; trying forbidden write\n");

    // volatile ensures the compiler emits the write.
    volatile int *pid = (volatile int *)USYSCALL;
    *pid = 0;

    printf("FAIL: write unexpectedly succeeded\n");
    exit(0);
  }

  int status = 0;
  if(wait(&status) != child){
    printf("FAIL: wait\n");
    exit(1);
  }

  if(status != -1){
    printf("FAIL: expected killed child, status=%d\n", status);
    exit(1);
  }

  if(ugetpid() != getpid()){
    printf("FAIL: parent PID changed\n");
    exit(1);
  }

  printf("USYSCALL READ-ONLY TEST PASSED\n");
  exit(0);
}