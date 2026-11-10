#include "kernel/types.h"
#include "user/user.h"

// Parse a hexadecimal address; reject invalid or overflowing input.
static int
parse_hex(char *s, uint64 *result)
{
  uint64 value = 0;

  if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;

  if(*s == '\0')
    return -1;

  while(*s){
    uint64 digit;
    char c = *s++;

    if(c >= '0' && c <= '9')
      digit = c - '0';
    else if(c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else if(c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else
      return -1;

    if(value > (~(uint64)0 - digit) / 16)
      return -1;

    value = value * 16 + digit;
  }

  *result = value;
  return 0;
}

int
main(int argc, char *argv[])
{
  uint64 va;

  if(argc != 2){
    printf("usage: uvmlookup <virtual_address>\n");
    exit(1);
  }

  if(parse_hex(argv[1], &va) < 0){
    printf("invalid hexadecimal address\n");
    exit(1);
  }

  printf("=== OUC uvmlookup ===\n");
  printf("query VA     : %p\n", (void *)va);

  if(vmlookup(va) < 0)
    exit(1);

  exit(0);
}