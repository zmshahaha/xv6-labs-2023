## lab util

pipe在不需要读时候关闭读端，不需要写关闭写端

pipe易错点：父进程关闭管道读不影响子进程，不要因为子进程要读而不关。也不要忘记父子进程均要关pipe读写端。

xargs实验中，由于使用中已经用了管道，内核已经用dup给到xargs。此时前面命令的输出已经是到xargs程序的输入，则xargs只需要使用read STDIN读前面命令输出。执行前面命令是命令行的事，不是xargs的事。

xargs中有如下几行

```c
  while ((read_num = read(0, &buf[read_len], sizeof(buf) - read_len)) != 0) {
    read_len += read_num;
  }
```
这是为了确保上个程序已经输出完。

本xargs中，由于输入参数以换行符为分隔为一个参数，对每个参数均执行一次，则子程序的参数个数与xargs相同(父程序的程序名换成子程序最后一个参数)。所以这里复用了父程序的argv
