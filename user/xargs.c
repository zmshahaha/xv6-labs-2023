#include <stddef.h>
#include "kernel/types.h"
#include "user.h"
#include "kernel/param.h"

#define MAXSIZE 128 

int main(int argc,char *argv[])
{
	char buf[MAXSIZE], *head, *tail;

	if (argc < 2) {
		fprintf(2, "usage: %s <command> <may have args...>)\n", argv[0]);
		exit(0);
	}

  int read_num, read_len = 0;
	while ((read_num = read(0, &buf[read_len], sizeof(buf) - read_len)) != 0) {
    read_len += read_num;
  }
  memmove(&argv[0], &argv[1], sizeof(argv[0]) * (argc - 1));

	tail = buf;
	while ((head = strchr(tail, '\n')) != NULL) {
		*head = '\0';
    head++;
		argv[argc - 1] = tail;
		tail = head;
	
		if(fork() == 0)
		{
			if(exec(argv[0], argv) < 0)
			{
				fprintf(2, "can't excute %s command\n", argv[0]);
				exit(1);
			}
		}
		else
		{
			wait(0); /* ensure it's in a right order */
		}
	}

	exit(0);
}
