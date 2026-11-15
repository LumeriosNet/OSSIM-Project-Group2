#include "common.h"
#include "syscall.h"
#include <stdio.h>

int
__sys_xxxhandler (struct krnl_t *krnl, uint32_t pid, struct sc_regs *regs)
{
  (void) krnl;
  (void) pid;

  if (regs == NULL)
    return -1;

  printf ("The first system call parameter " FORMAT_ARG "\n", regs->a1);
  return 0;
}
