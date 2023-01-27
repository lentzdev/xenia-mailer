/* Sample to show how to read the first user from user.bbs */

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "max.h"

char * yes_no(int flag)
{
  return flag ? "yes" : "no";
}

int main(void)
{
  struct _usr usr;
  int fd;

  if ((fd=open("user.bbs", O_RDONLY | O_BINARY))==-1)
  {
    printf("Error reading user file!\n");
    return 1;
  }

  read(fd, (char *)&usr, sizeof usr);
  close(fd);

  printf("name     = %s\n", usr.name);
  printf("city     = %s\n", usr.city);

  printf("ibmchars = %s\n", yes_no(usr.bits2 & BITS2_IBMCHARS));
  printf("fsr      = %s\n", yes_no(usr.bits & BITS_FSR));
  return 0;
}

