#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pipefd[2], pid;
  char buf;

  if (pipe(pipefd) != 0) {
    fprintf(2, "pipe error");
    exit(1);
  }

  if ((pid = fork()) == 0) {
    read(pipefd[0], &buf, 1);
    printf("%d: received ping\n", getpid());
    buf = 'a';
    write(pipefd[1], &buf, 1);
    close(pipefd[0]);
    close(pipefd[1]);
  } else {
    buf = 'b';
    write(pipefd[1], &buf, 1);
    read(pipefd[0], &buf, 1);
    printf("%d: received pong\n", getpid());
    close(pipefd[0]);
    close(pipefd[1]);
  }
  exit(0);
}
