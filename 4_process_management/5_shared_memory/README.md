# Shared Memory
## 1. 功能要求
>编写程序sender，它创建一个共享内存，然后等待用户通过终端输入一串字符，并将这串字符通过共享内存发送给receiver；
最后，它等待receiver的应答，等到应答消息后，将接收到的应答信息显示在终端屏幕上，删除共享内存，结束程序的运行。
编写receiver程序，它通过共享内存接收来自sender的消息，将消息显示在终端屏幕上，然后再通过该共享内存向sender发送一个应答消息“over”，结束程序的运行。
使用有名信号量或System V信号量实现两个进程对共享内存的互斥使用。


## 2. 编译
```
make
```

## 3. 运行
```
./
```

## 4. 输出
```
```