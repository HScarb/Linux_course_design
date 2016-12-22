#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

/* define const */
#define BLOCKSIZE           1024    // 磁盘块大小
#define SIZE                1024000 // 虚拟磁盘空间大小
#define END                 65535   // FAT中的文件结束标志
#define FREE                0       // FAT中盘块空闲标志
#define MAXOPENFILE         10      // 最多同时打开的文件个数

#define MAX_TEXT_SIZE       10000

/* define data structures */
/**
 * @name
 * File Control Block |　文件控制块
 * @note
 * 用户存储文件的描述和控制信息的数据结构
 * 常用的有FCB和iNode,在FAT文件系统中使用的是FCB
 * 文件与文件控制块一一对应关系。
 */
typedef struct FCB{
    char filename[8];               // 文件名
    char exname[3];                 // 文件扩展名
    unsigned char attribute;        // 文件属性字段
    unsigned short time;            // 文件创建时间
    unsigned short date;            // 文件创建日期
    unsigned short first;           // 文件起始盘块号
    unsigned long length;           // 文件长度
    char free;                      // 表示目录项是否为空，0表示空，1表示已分配
}

/**
 * FAT : File Allocation Table
 * 文件分配表
 */
typedef struct FAT{
    unsigned short id;
}fat;

// 用户打开文件表
typedef struct USEROPEN {
    char filename[8];               // 文件名
    char exname[3];                 // 文件扩展名
    unsigned char attribute;        // 文件属性字段
    unsigned short time;            // 文件创建时间
    unsigned short date;            // 文件创建日期
    unsigned short first;           // 文件起始盘块号
    unsigned long length;           // 文件长度（对数据文件是字节数，对目录文件可以是目录项个数）
    char free;                      // 表示目录项是否为空，若值为0，表示空，值为1，表示已分配

    int dirno;                      // 相应打开文件的目录项在父目录文件中的盘块号
    int diroff;                     // 相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号
    char dir[80];                   // 相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开
    int count;                      // 读写指针在文件中的位置
    char fcbstate;                  // 是否修改了文件的FCB的内容，如果修改了置为1，否则为0
    char topenfile;                 // 表示该用户打开表项是否为空，若值为0，表示为空，否则表示已被某打开文件占据
} useropen;

/**
 * 引导块
 */
typedef struct BLOCK0{
    char magic_number[8];           // 文件系统的魔数
    char information[200];
    unsigned short root;            // 根目录文件的起始盘块号
    unsigned char * startblock;     // 虚拟盘块上数据区开始位置
}block0;

/* global variables */
unsigned char * myvhard;            // 指向虚拟磁盘的起始地址
useropen openfilelist[MAXOPENFILE]; // 用户打开文件表数组
int currfd;                         // 记录当前用户打开文件表项下标
unsigned char * startp;             // 记录虚拟磁盘上数据区开始位置
char * fileName = "myfilesys.txt";  // 数据存储路径
unsigned char buffer[SIZE];         // 缓冲区

/* functions */
void startSys();                    // 进入文件系统
void exitsys();                     // 退出文件系统
void my_format();                   // 磁盘格式化函数
void my_mkdir(char *dirname);       // 创建子目录
void my_rmdir(char *dirname);       // 删除子目录
void my_ls();                       // 显示目录中的内容
void my_cd(char *dirname);          // 用于更改当前目录
int  my_create(char *filename);     // 创建文件
void my_rm(char *filename);         // 删除文件
int  my_open(char *filename);       // 打开文件
int  my_close(int fd);              // 关闭文件
int  my_write(int fd);              // 写文件
int  my_read(int fd);               // 读文件
int  do_write(int fd, char *text, int len, char wstyle);
int  do_read(int fd, int len, char *text);

unsigned short getFreeBLOCK();      // 获取一个空闲的磁盘块
int getFreeOpenfilelist();          // 获取一个空闲的文件打开表项
int find_father_dir(int fd);        // 寻找一个打开文件的父目录打开文件

/** startSys
① 申请虚拟磁盘空间；
② 使用c语言的库函数fopen()打开myfsys文件：若文件存在，则转③；若文件不存在，则创建之，转⑤
③ 使用c语言的库函数fread()读入myfsys文件内容到用户空间中的一个缓冲区中，并判断其开始的8个字节内容是否为“10101010”（文件系统魔数），如果是，则转④；否则转⑤；
④ 将上述缓冲区中的内容复制到内存中的虚拟磁盘空间中；转⑦
⑤ 在屏幕上显示“myfsys文件系统不存在，现在开始创建文件系统”信息，并调用my_format()对①中申请到的虚拟磁盘空间进行格式化操作。转⑥；
⑥ 将虚拟磁盘中的内容保存到myfsys文件中；转⑦
⑦ 使用c语言的库函数fclose()关闭myfsys文件；
⑧ 初始化用户打开文件表，将表项0分配给根目录文件使用，并填写根目录文件的相关信息，由于根目录没有上级目录，所以表项中的dirno和diroff分别置为5（根目录所在起始块号）和0；并将ptrcurdir指针指向该用户打开文件表项。
⑨ 将当前目录设置为根目录。
 */
void startSys()
{
    /**
     * 1. 寻找
    myvhard = (unsigned char *)malloc(SIZE);
    
}