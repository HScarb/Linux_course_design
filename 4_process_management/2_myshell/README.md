# Shell & Fork
## 1. 功能要求

>编写三个不同的程序cmd1.c，cmd2.c，cmd3.c，每个程序输出一句话，分别编译成可执行文件cmd1，cmd2，cmd3。
然后再编写一个程序，模拟shell程序的功能，能根据用户输入的字符串（表示相应的命令名），去为相应的命令创建子进程并让它去执行相应的程序，
而父进程则等待子进程结束，然后再等待接收下一条命令。
如果接收到的命令为exit，则父进程结束；如果接收到的命令是无效命令，则显示“Command not found”，继续等待。

## 2. 编译
```
make
```

## 3. 运行
```
./myshell
```

## 4. 命令
```
cmd1
cmd2
cmd3
exit
```

## 5. 输出
```
# ./myshell 

*Please Input your command:
cmd1
Process 1 is running.
*Wating for next command...

*Please Input your command:
cmd2
Process 2 is running.
*Wating for next command...

*Please Input your command:
cmd3
Process 3 is running.
*Wating for next command...

*Please Input your command:
exit
```