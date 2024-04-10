#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

char*
fmtname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

int
find(char *dir, char *file)
{
  char buf[512], *p, *fmt_name;
  int fd, ret = 0;
  struct dirent de;
  struct stat st;

  if((fd = open(dir, O_RDONLY)) < 0){
    fprintf(2, "find: cannot open %s\n", dir);
    return 1;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", dir);
    close(fd);
    return 1;
  }

  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    fmt_name = fmtname(dir);
    if (strcmp(fmt_name, file) == 0) {
        printf("%s\n", dir);
    }
    return 0;

  case T_DIR:
    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
      fprintf(2, "find: dir path too long\n");
      return 1;
    }
    strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      fmt_name = fmtname(buf);
      if ((strcmp(fmt_name, ".") == 0) || (strcmp(fmt_name, "..") == 0)) {
        continue;
      }
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      switch (st.type) {
        case T_DEVICE:
        case T_FILE:
          if (strcmp(fmt_name, file) == 0) {
            printf("%s\n", buf);
          }
          break;
        case T_DIR:
          ret = find(buf, file);
          break;
      }
    }
    break;
  }
  close(fd);
  return ret;
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "Usage: find dirname filename\n");
    exit(1);
  }

  exit(find(argv[1], argv[2]));
}
