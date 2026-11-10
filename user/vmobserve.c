#include "kernel/types.h"
#include "user/user.h"

int global_var = 123;

int
main(void)
{
  int local_var = 456;
  uint64 va = (uint64)&local_var;

  printf("\n===OUC===c1: address analysis ===\n");

  printf("stack address : %p\n", (void *)va);
  printf("VPN[2]       : %d\n", (int)((va >> 30) & 0x1FF));
  printf("VPN[1]       : %d\n", (int)((va >> 21) & 0x1FF));
  printf("VPN[0]       : %d\n", (int)((va >> 12) & 0x1FF));
  printf("offset       : %p\n", (void *)(va & 0xFFF));

  printf("\n===OUC===c1: address analysis Done ===\n");

  printf("\n===OUC===c1: vmprint() ===\n");
  if(vmprint() < 0){
    printf("vmprint failed\n");
    exit(1);
  }
  printf("\n===OUC===c1: vmprint() Done ===\n");

  exit(0);
}
