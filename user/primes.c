#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
child(int *pipefd) {
  int prime, num, pid;
  int new_pipefd[2];

  close(pipefd[1]);

  if (read(pipefd[0], &prime, sizeof(prime)) == 0) {
    close(pipefd[0]);
    return;
  }

  printf("prime %d\n", prime);

  // dont fork and pipe now to save fd, even its make code dirty
  while (1) {
    if (read(pipefd[0], &num, sizeof(num)) == 0) {
      close(pipefd[0]);
      return;
    }
    if (num % prime == 0) {
      continue;
    }
    break;
  }

  if (pipe(new_pipefd) != 0) {
    fprintf(2, "pipe error");
    exit(1);
  }

  if ((pid = fork()) == 0) {
    child(new_pipefd);
  } else {
    write(new_pipefd[1], &num, sizeof(num));
    while (read(pipefd[0], &num, sizeof(num))) {
      if (num % prime == 0) {
        continue;
      }
      write(new_pipefd[1], &num, sizeof(num));
    }
    close(new_pipefd[1]);
  }
}

int
main(int argc, char *argv[])
{
  int pipefd[2], pid, num;

  if (pipe(pipefd) != 0) {
    fprintf(2, "pipe error");
    exit(1);
  }

  if ((pid = fork()) == 0) {
    child(pipefd);
  } else {
    close(pipefd[0]);
    printf("prime 2\n");

    for (num = 3; num < 35; num++) {
      if (num % 2 == 0) {
        continue;
      }
      write(pipefd[1], &num, sizeof(num));
    }
    close(pipefd[1]);
  }
  wait(0);
  exit(0);
}
