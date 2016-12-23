#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* define const */
#define BLOCKSIZE           1024    // 磁盘块大小
#define SIZE                1024000 // 虚拟磁盘空间大小
#define END                 65535   // FAT中的文件结束标志
#define FREE                0       // FAT中盘块空闲标志
#define ROOTBLOCKNUM		2		// 根目录所占盘块数
#define MAXOPENFILE         10      // 最多同时打开的文件个数

#define MAX_TEXT_SIZE       10000
#define FILENAME_SIZE		8		// 文件名长度
#define EXNAME_SIZE			4		// 文件扩展名长度

/* define data structures */
/**
* File Control Block |　文件控制块 | 仿照FAT16
用于记录文件的描述和控制信息，每个文件设置一个FCB，它也是文件的目录项的内容。
*/
typedef struct FCB {
	char filename[FILENAME_SIZE];   // 文件名
	char exname[EXNAME_SIZE];		// 文件扩展名
	unsigned char attribute;        // 文件属性字段，值为0时表示目录文件，值为1时表示数据文件
	unsigned short time;            // 文件创建时间
	unsigned short date;            // 文件创建日期
	unsigned short first;           // 文件起始盘块号
	unsigned long length;           // 文件长度(字节数)
	char free;                      // 表示目录项是否为空，0表示空，1表示已分配
} fcb;

/**
* FAT : File Allocation Table
* 文件分配表
在本实例中，文件分配表有两个作用：一是记录磁盘上每个文件所占据的磁盘块的块号；
	二是记录磁盘上哪些块已经分配出去了，哪些块是空闲的，即起到了位示图的作用。
	若FAT中某个表项的值为FREE，则表示该表项所对应的磁盘块是空闲的；
	若某个表项的值为END，则表示所对应的磁盘块是某文件的最后一个磁盘块；
	若某个表项的值是其他值，则该值表示某文件的下一个磁盘块的块号。
	为了提高系统的可靠性，本实例中设置了两张FAT表，它们互为备份，每个FAT占据两个磁盘块。
*/
typedef struct FAT {
	unsigned short id;				// 磁盘块的状态(空闲的，最后的，下一个)
} fat;

/**	用户打开文件表
当打开一个文件时，必须将文件的目录项中的所有内容全部复制到内存中，
同时还要记录有关文件操作的动态信息，如读写指针的值等。
在本实例中实现的是一个用于单用户单任务系统的文件系统，
为简单起见，我们把用户文件描述符表和内存FCB表合在一起，称为用户打开文件表，
表项数目为10，即一个用户最多可同时打开10个文件。
然后用一个数组来描述，则数组下标即某个打开文件的描述符。
另外，我们在用户打开文件表中还设置了一个字段“char dir[80]”，
用来记录每个打开文件所在的目录名，以方便用户打开不同目录下具有相同文件名的不同文件。
 */
typedef struct USEROPEN {
	char filename[FILENAME_SIZE];	// 文件名
	char exname[EXNAME_SIZE];		// 文件扩展名
	unsigned char attribute;        // 文件属性字段，值为0时表示目录文件，值为1时表示数据文件
	unsigned short time;            // 文件创建时间
	unsigned short date;            // 文件创建日期
	unsigned short first;           // 文件起始盘块号
	unsigned long length;           // 文件长度（对数据文件是字节数，对目录文件可以是目录项个数）
	char free;                      // 表示目录项是否为空，若值为0，表示空，值为1，表示已分配
	// 下面设置的dirno和diroff记录了相应打开文件的目录项在父目录文件中的位置，
	// 这样如果该文件的fcb被修改了，则要写回父目录文件时比较方便
	int dirno;                      // 相应打开文件的目录项在父目录文件中的盘块号
	int diroff;                     // 相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号
	char dir[80];                   // 相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开
	int father;						// 父目录在打开文件表项的位置
	int count;                      // 读写指针在文件中的位置
	char fcbstate;                  // 是否修改了文件的FCB的内容，如果修改了置为1，否则为0
	char topenfile;                 // 表示该用户打开表项是否为空，若值为0，表示为空，否则表示已被某打开文件占据
} useropen;

/**
* 引导块
在引导块中主要存放逻辑磁盘的相关描述信息，
比如磁盘块大小、磁盘块数量、文件分配表、根目录区、数据区在磁盘上的起始位置等。
如果是引导盘，还要存放操作系统的引导信息。
本实例是在内存的虚拟磁盘中创建一个文件系统，因此所包含的内容比较少，
只有磁盘块大小、磁盘块数量、数据区开始位置、根目录文件开始位置等。
*/
typedef struct BLOCK0 {
	char magic_number[8];           // 文件系统的魔数
	char information[200];
	unsigned short root;            // 根目录文件的起始盘块号
	unsigned char * startblock;     // 虚拟盘块上数据区开始位置
}block0;

/* global variables */
unsigned char * myvhard;            // 指向虚拟磁盘的起始地址 / unsigned char = byte
useropen openfilelist[MAXOPENFILE]; // 用户打开文件表数组
//int currfd;                         // current field/记录当前用户打开文件表项下标
int curdir;							// 用户打开文件表中的当前目录所在打开文件表项的位置(下标)
char currentdir[80];				// 记录当前目录的目录名(包括目录的路径)
unsigned char * startp;             // 记录虚拟磁盘上数据区开始位置
char * myFileName = "myfilesys.txt";  // 数据存储路径
unsigned char buffer[SIZE];         // 缓冲区

/* functions */

void my_startsys();                 // 进入文件系统
void my_exitsys();                  // 退出文件系统
void my_format();                   // 磁盘格式化函数
void my_mkdir(char *dirname);       // 创建子目录
void my_rmdir(char *dirname);       // 删除子目录
void my_ls();                       // 显示目录中的内容
void my_cd(char *dirname);          // 用于更改当前目录
void  my_create(char *filename);     // 创建文件
void my_rm(char *filename);         // 删除文件
int  my_open(char *filename);       // 打开文件
int  my_close(int fd);              // 关闭文件
int  my_write(int fd);              // 写文件
int  my_read(int fd, int len);      // 读文件
int  do_write(int fd, char *text, int len, char wstyle);
int  do_read(int fd, int len, char *text);

unsigned short getFreeBLOCK();      // 获取一个空闲的磁盘块
int getFreeOpenfilelist();          // 获取一个空闲的文件打开表项
//int find_father_dir(int fd);        // 寻找一个打开文件的父目录打开文件

unsigned short getFreeBLOCK()
{
	unsigned short i;
	fat *fat1, *fatptr;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	for (i = 7; i < SIZE / BLOCKSIZE; i++)		// i = 7,数据区从7开始
	{
		fatptr = fat1 + i;
		if (fatptr->id == FREE)
			return i;
	}
	return -1;
}

int getFreeOpenfilelist()
{
	int i;
	for (i = 0; i < MAXOPENFILE; i++)
	{
		if (openfilelist[i].topenfile == 0)
			return i;
	}
	printf("[Error] Open too many files!\n");
	return -1;
}

/* 
my_startsys | 进入文件系统函数
1. 寻找myfsys.txt文件，如果存在而且开头是文件魔数，就读取到myvhard，
	  否则创建文件并写入初始化信息
2. 设置用户打开文件表的第一个表项，内容为根目录内容，即默认打开根目录
3. 初始化一堆全局变量
*/
void my_startsys()
{
	/**
	① 申请虚拟磁盘空间；
	② 使用c语言的库函数fopen()打开myfsys文件：若文件存在，则转③；若文件不存在，则创建之，转⑤
	③ 使用c语言的库函数fread()读入myfsys文件内容到用户空间中的一个缓冲区中，
	并判断其开始的8个字节内容是否为“10101010”（文件系统魔数），如果是，则转④；否则转⑤；
	④ 将上述缓冲区中的内容复制到内存中的虚拟磁盘空间中；转⑦
	⑤ 在屏幕上显示“myfsys文件系统不存在，现在开始创建文件系统”信息，
	并调用my_format()对①中申请到的虚拟磁盘空间进行格式化操作。转⑥；
	⑥ 将虚拟磁盘中的内容保存到myfsys文件中；转⑦
	⑦ 使用c语言的库函数fclose()关闭myfsys文件；
	⑧ 初始化用户打开文件表，将表项0分配给根目录文件使用，并填写根目录文件的相关信息，
	由于根目录没有上级目录，所以表项中的dirno和diroff分别置为5（根目录所在起始块号）和0；
	并将ptrcurdir指针指向该用户打开文件表项。
	⑨ 将当前目录设置为根目录。
	*/
	FILE * file;
	int i;
	myvhard = (unsigned char *)malloc(SIZE);
	// 如果文件不存在或者开头不是文件魔数，重新创建文件
	if ((file = fopen(myFileName, "r")) != NULL)
	{
		fread(buffer, SIZE, 1, file);			// 将二进制文件读取到缓冲区
		fclose(file);
		if(memcmp(buffer, "10101010", 8) == 0)
		{
			memcpy(myvhard, buffer, SIZE);
			printf("myfsys文件读取成功\n");
		}
		else									// 有文件但是开头不是文件魔数
		{
			printf("myfsys文件系统不存在，现在创建文件系统\n");
			my_format();
			memcpy(buffer, myvhard, SIZE);
		}
	}
	else
	{
		printf("myfsys文件系统不存在，现在开始创建文件系统\n");
		my_format();
		memcpy(buffer, myvhard, SIZE);
	}

	/**
	 *	⑧ 初始化用户打开文件表，将表项0分配给根目录文件使用，并填写根目录文件的相关信息，
	 *	  由于根目录没有上级目录，所以表项中的dirno和diroff分别置为5（根目录所在起始块号）和0；
	 *	  并将ptrcurdir指针指向该用户打开文件表项。
	 *  ⑨ 将当前目录设置为根目录。
	 */
	fcb *root;
	root = (fcb*)(myvhard + 5 * BLOCKSIZE);		// 根目录首地址
	strcpy(openfilelist[0].filename, root->filename);
	strcpy(openfilelist[0].exname, root->exname);
	openfilelist[0].attribute = root->attribute;
	openfilelist[0].time = root->time;
	openfilelist[0].date = root->date;
	openfilelist[0].first = root->first;
	openfilelist[0].length = root->length;
	openfilelist[0].free = root->free;
	openfilelist[0].dirno = 5;					// 第6块
	openfilelist[0].diroff = 0;
	strcpy(openfilelist[0].dir, "root\\");		// 打开文件所在的目录名
	openfilelist[0].father = 0;
	openfilelist[0].count = 0;					// 读写指针的位置
	openfilelist[0].fcbstate = 0;
	openfilelist[0].topenfile = 1;				// 设打开表项
	for (i = 1; i < MAXOPENFILE; i++)			// 设其他表项为非打开状态
		openfilelist[i].topenfile = 0;
	
	curdir = 0;									// 当前表项
	strcpy(currentdir, "root\\");				// 当前目录名
	startp = ((block0 *)myvhard)->startblock;	// 数据区开始位置
}

/*
my_exitsts | 退出文件系统函数
*/
void my_exitsys()
{
	/*
	① 使用C库函数fopen()打开磁盘上的myfsys文件；
	② 将虚拟磁盘空间中的所有内容保存到磁盘上的myfsys文件中；
	③ 使用c语言的库函数fclose()关闭myfsys文件；
	④ 撤销用户打开文件表，释放其内存空间
	⑤ 释放虚拟磁盘空间。
	*/
	FILE *file;
	while (curdir)
		curdir = my_close(curdir);			// 关闭所有打开的目录
	file = fopen(myFileName, "w");
	fwrite(myvhard, SIZE, 1, file);
	fclose(file);
	free(myvhard);
}


/*
my_format | 磁盘格式化函数
1. 设置引导块(一个盘块)
2. 设置FAT1(两个盘块) FAT2(两个盘块)
3. 设置根目录文件的两个特殊目录项.和..(根目录文件占一个盘块，两个目录项写在这个盘块里面)
*/
void my_format()
{
	/**
	① 将虚拟磁盘第一个块作为引导块，开始的8个字节是文件系统的魔数，记为“10101010”；
	在之后写入文件系统的描述信息，如FAT表大小及位置、根目录大小及位置、盘块大小、盘块数量、数据区开始位置等信息；
	② 在引导块后建立两张完全一样的FAT表，用于记录文件所占据的磁盘块及管理虚拟磁盘块的分配，
	每个FAT占据两个磁盘块；对于每个FAT中，前面5个块设置为已分配，后面995个块设置为空闲；
	③ 在第二张FAT后创建根目录文件root，将数据区的第1块（即虚拟磁盘的第6块）分配给根目录文件，
	在该磁盘上创建两个特殊的目录项：“.”和“..”，其内容除了文件名不同之外，其他字段完全相同。
	*/
	FILE * file;
	fat *fat1, *fat2;
	block0 *boot;
	time_t now;
	struct tm * nowtime;
	fcb *root;
	int i;

	boot = (block0 *)myvhard;					// 引导块位于磁盘首部
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	root = (fcb*)(myvhard + 5 * BLOCKSIZE);
	strcpy(boot->magic_number, "10101010");
	strcpy(boot->information, "Scarb文件系统,外存分配方式:FAT; 磁盘空间管理:结合于FAT的位示图; 结构:单用户多级目录结构");
	boot->root = 5;
	boot->startblock = (unsigned char *)root;
	for (i = 0; i < 5; i++)
	{
		fat1->id = END;
		fat2->id = END;
		fat1++;
		fat2++;
	}
	// 根目录区在第6、7块
	fat1->id = 6;
	fat2->id = 6;
	fat1++, fat2++;
	fat1->id = END;
	fat2->id = END;
	fat1++, fat2++;
	// 数据区磁盘块空闲
	for (i = 7; i < SIZE / BLOCKSIZE; i++)
	{
		fat1->id = FREE;
		fat2->id = FREE;
		fat1++, fat2++;
	}
	now = time(NULL);							// 返回一个时间戳
	nowtime = localtime(&now);					// 转换为本地时间
	// 建立目录"."
	strcpy(root->filename, ".");
	strcpy(root->exname, "");
	root->attribute = 0;						// 0,目录文件
	root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	root->first = 5;
	root->length = 2 * sizeof(fcb);
	root->free = 1;
	// 建立目录".."
	root++;
	now = time(NULL);
	nowtime = localtime(&now);
	strcpy(root->filename, "..");
	strcpy(root->exname, "");
	root->attribute = 0;
	root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	root->first = 5;
	root->length = 2 * sizeof(fcb);
	root->free = 1;

	// 写回磁盘
	file = fopen(myFileName, "w");
	fwrite(myvhard, SIZE, 1, file);
	fclose(file);
}

/*
my_cd | 更改当前目录函数
① 调用my_open()打开指定目录名的父目录文件，并调用do_read()读入该父目录文件内容到内存中；
② 在父目录文件中检查新的当前目录名是否存在，如果存在则转③，否则返回，并显示出错信息；
③ 调用my_close()关闭①中打开的父目录文件；
④ 调用my_close()关闭原当前目录文件；
⑤ 如果新的当前目录文件没有打开，则打开该目录文件；并将ptrcurdir指向该打开文件表项；
⑥ 设置当前目录为该目录。
总结:
如果当前是目录文件下,那么需要把这个目录文件读取到buf里, 然后检索这个文件里的fcb有没有匹配dirname的目录项(而且必须是目录文件)
如果有,那就在openfilelist里取一个打开文件表项,把这个dirname这个目录文件的fcb写进去,然后切换currfd=fd
这样就算是打开一个目录
*/
void my_cd(char* dirname)
{
	char *dir;
	int fd;
	dir = strtok(dirname, "\\");
	if (strcmp(dir, ".") == 0)					// 如果转到当前目录，不做处理
		return;									
	else if(strcmp(dir, "..") == 0)				// 如果转到上层目录
	{
		if (curdir)
			curdir = my_close(curdir);			// 关闭当前目录，返回值为父目录在打开表的位置
		return;
	}
	else if(strcmp(dir, "root") == 0)
	{
		while (curdir)							// 循环关闭root下打开的文件
		{
			curdir = my_close(curdir);
		}
		dir = strtok(NULL, "\\");
	}
	while (dir)
	{
		fd = my_open(dir);
		if (-1 != fd)							// 成功转入路径
			curdir = fd;						// 设置当前目录为打开的目录
		else
		{
			printf("[Error]my_cd: Can not cd this dir.\n");
			return;
		}
		dir = strtok(NULL, "\\");				// 剩下的路径
	}
}

/*
do_read | 实际读取文件函数
功能：被my_read()调用，读出指定文件中从读写指针开始的长度为len的内容到用户空间的text中。
输入：
	fd：		open()函数的返回值，文件的描述符；
	len:	要求从文件中读出的字节数。
	text:	指向存放读出数据的用户区地址
输出：实际读出的字节数。
*/
int do_read(int fd, int len, char* text)
{
	/*
	① 使用malloc()申请1024B空间作为缓冲区buf，申请失败则返回-1，并显示出错信息；
	② 将读写指针转化为逻辑块块号及块内偏移量off，
	  利用打开文件表表项中的首块号查找FAT表，找到该逻辑块所在的磁盘块块号；
	  将该磁盘块块号转化为虚拟磁盘上的内存位置；
	③ 将该内存位置开始的1024B（一个磁盘块）内容读入buf中；
	④ 比较buf中从偏移量off开始的剩余字节数是否大于等于应读写的字节数len，
	  如果是，则将从off开始的buf中的len长度的内容读入到text[]中；
	  否则，将从off开始的buf中的剩余内容读入到text[]中；
	⑤ 将读写指针增加④中已读字节数，将应读写的字节数len减去④中已读字节数，若len大于0，则转②；否则转⑥；
	⑥ 使用free()释放①中申请的buf。
	⑦ 返回实际读出的字节数。
	*/
	fat *fat1, *fatptr;
	unsigned char *buf, *blkptr;
	unsigned short blkno, blkoff;
	int i, ll;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	buf = (unsigned char *)malloc(BLOCKSIZE);
	if(buf == NULL)
	{
		printf("[Error]do_read: malloc error!\n");
		return -1;
	}
	blkno = openfilelist[fd].first;						// 文件的起始块号
	blkoff = openfilelist[fd].count;					// 块内偏移
	if(blkoff >= openfilelist[fd].length)
	{
		printf("[Error]do_read: Read out of range!\n");
		free(buf);
		return -1;
	}
	fatptr = fat1 + blkno;
	while (blkoff >= BLOCKSIZE)		// 块内偏移大于块的大小则将块号置为下一块的块号，调整偏移
	{
		blkno = fatptr->id;
		blkoff = blkoff - BLOCKSIZE;
		fatptr = fat1 + blkno;
	}
	// 读取
	ll = 0;
	while (ll < len)
	{
		blkptr = (unsigned char *)(myvhard + blkno * BLOCKSIZE);
		for (i = 0; i < BLOCKSIZE; i++)					// 将最后一块盘块的内容读取到buf中
			buf[i] = blkptr[i];
		for (; blkoff < BLOCKSIZE; blkoff++)			// 在buf中，从读写指针处开始读数据
		{
			text[ll++] = buf[blkoff];					// 将缓冲区中内容读到text中
			openfilelist[fd].count++;					// 在读的过程中，读写指针也跟着变化
			if (ll == len || openfilelist[fd].count == openfilelist[fd].length)
				break;
		}
		if (ll < len && openfilelist[fd].count != openfilelist[fd].length)	// 一个盘块的内容不够读
		{
			blkno = fatptr->id;
			if (blkno == END)							// 该文件没有下一个盘块，停止
				break;
			blkoff = 0;									// 在下一个新的盘块中，读写指针要重头开始读
			fatptr = fat1 + blkno;						// 实际地址
		}
	}
	text[ll] = '\0';
	free(buf);

	return ll;
}

/*
do_write | 实际写文件函数
功能:被写文件函数my_write()调用，用来将键盘输入的内容写到相应的文件中去。
输入:
	fd:		open()函数的返回值，文件的描述符；
	text:	指向要写入的内容的指针；
	len:	本次要求写入字节数
	wstyle:	写方式
输出:实际写入的字节数。
*/
int do_write(int fd, char* text, int len, char wstyle)
{
	/*
	① 用malloc()申请1024B的内存空间作为读写磁盘的缓冲区buf，申请失败则返回-1，并显示出错信息；
	② 将读写指针转化为逻辑块块号和块内偏移off，
	  并利用打开文件表表项中的首块号及FAT表的相关内容将逻辑块块号转换成对应的磁盘块块号blkno；
	  如果找不到对应的磁盘块，则需要检索FAT为该逻辑块分配一新的磁盘块，
	  并将对应的磁盘块块号blkno登记到FAT中，若分配失败，则返回-1，并显示出错信息；
	③ 如果是覆盖写，或者如果当前读写指针所对应的块内偏移off不等于0，
	  则将块号为blkno的虚拟磁盘块全部1024B的内容读到缓冲区buf中；否则便用ASCII码0清空buf；
	④ 将text中未写入的内容暂存到缓冲区buff的第off字节开始的位置，直到缓冲区满，
	  或者接收到结束字符CTR+Z为止；将本次写入字节数记录到tmplen中；
	⑤ 将buf中1024B的内容写入到块号为blkno的虚拟磁盘块中；
	⑥ 将当前读写指针修改为原来的值加上tmplen；并将本次实际写入的字节数增加tmplen；
	⑦ 如果tmplen小于len，则转②继续写入；否则转⑧；
	⑧ 返回本次实际写入的字节数。
	*/
	fat *fat1, *fat2, *fatptr1, *fatptr2;
	unsigned char *buf, *blkptr;
	unsigned short blkno, blkoff;
	int i, ll;
	fat1 = (fat *)(myvhard + BLOCKSIZE);
	fat2 = (fat *)(myvhard + 3 * BLOCKSIZE);
	buf = (unsigned char *)malloc(BLOCKSIZE);
	if(buf == NULL)
	{
		printf("[Error]do_write: malloc error!\n");
		return -1;
	}
	
	blkno = openfilelist[fd].first;
	blkoff = openfilelist[fd].count;
	fatptr1 = fat1 + blkno;
	fatptr2 = fat2 + blkno;
	while (blkoff >= BLOCKSIZE)		// 当读写指针大于等于盘块大小，快内偏移大于块的大小(一个磁盘放不下所写文件)
	{
		blkno = fatptr1->id;					// 下一块号
		if(blkno == END)						// 该磁盘块是结束块，单独，没有下一块
		{
			blkno = getFreeBLOCK();				// 寻找空闲块
			if (blkno == -1)					// 没有空闲快
			{
				printf("[Error]do_write: Can't find free blocks.\n");
				free(buf);
				return -1;
			}
			fatptr1->id = blkno;				// 写入起始块号
			fatptr2->id = blkno;
			fatptr1 = fat1 + blkno;
			fatptr2 = fat2 + blkno;
			fatptr1->id = END;
			fatptr2->id = END;
		}
		else									// 存在下一块
		{
			fatptr1 = fat1 + blkno;
			fatptr2 = fat2 + blkno;
		}
		blkoff = blkoff - BLOCKSIZE;	// 调整快内偏移，让blkoff定位到文件最后一个磁盘块的读写位置
	}
	// 写
	ll = 0;
	while (ll < len)
	{
		blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE);		// 指向虚拟磁盘中要写入的盘块
		for (i = 0; i < BLOCKSIZE; i++)			// 把盘块中原有的数据拷贝到缓冲区buf中
			buf[i] = blkptr[i];
		for (; blkoff < BLOCKSIZE; blkoff++)	// 把text指针指向的内容拷贝到缓冲区buf中
		{
			buf[blkoff] = text[ll++];
			openfilelist[fd].count++;
			if (ll == len)
				break;
		}
		for (i = 0; i < BLOCKSIZE; i++)			// 把缓冲区buf中的内容拷贝到要写入的虚拟磁盘块中
			blkptr[i] = buf[i];
		if(ll < len)							// 如果一个磁盘块写不下，则再分配一个磁盘块
		{
			blkno = fatptr1->id;
			if(blkno == END)
			{
				blkno = getFreeBLOCK();
				if (blkno == -1)
				{
					printf("[Error]do_write: Can't find free blocks.\n");
					return -1;
				}
				fatptr1->id = blkno;
				fatptr2->id = blkno;
				fatptr1 = fat1 + blkno;
				fatptr2 = fat2 + blkno;
				fatptr1->id = END;
				fatptr2->id = END;
			}
			else
			{
				fatptr1 = fat1 + blkno;
				fatptr2 = fat2 + blkno;
			}
			blkoff = 0;
		}
	}
	if (openfilelist[fd].count > openfilelist[fd].length)		// 若读写指针大于原来文件的长度，则修改文件的长度
		openfilelist[fd].length = openfilelist[fd].count;
	openfilelist[fd].fcbstate = 1;								// 已修改
	free(buf);
	return ll;
}

/*
my_read | 读文件函数
功能:读出指定文件中从读写指针开始的长度为len的内容到用户空间中。
输入:
fd:		open()函数的返回值，文件的描述符；
len:	要从文件中读出的字节数。
输出:实际读出的字节数。
*/
int my_read(int fd, int len)
{
	/*
	① 定义一个字符型数组text[len]，用来接收用户从文件中读出的文件内容；
	② 检查fd的有效性（fd不能超出用户打开文件表所在数组的最大下标），如果无效则返回-1，并显示出错信息；
	③ 调用do_read()将指定文件中的len字节内容读出到text[]中；
	④ 如果do_read()的返回值为负，则显示出错信息；否则将text[]中的内容显示到屏幕上；
	⑤ 返回。
	*/
	char text[MAX_TEXT_SIZE];
	int ll;
	if (fd < 0 || fd > MAXOPENFILE)
	{
		printf("[Error]my_read: This file don't exist.\n");
		return -1;
	}
	openfilelist[fd].count = 0;
	ll = do_read(fd, len, text);
	if (ll != -1)
		printf("%s\n", text);
	else
		printf("[Error]my_read: Read error!\n");
	return ll;
}

/*
my_mkdir | 创建子目录函数
功能:在当前目录下创建名为dirname的子目录。
输入:
	dirname:新建目录的目录名。
*/
void my_mkdir(char* dirname)
{
	/*
	① 调用do_read()读入当前目录文件内容到内存，检查当前目录下新建目录文件是否重名，若重名则返回，并显示错误信息；
	② 为新建子目录文件分配一个空闲打开文件表项，如果没有空闲表项则返回-1，并显示错误信息；
	③ 检查FAT是否有空闲的盘块，如有则为新建目录文件分配一个盘块，
	  否则释放①中分配的打开文件表项，返回，并显示错误信息；
	④ 在当前目录中为新建目录文件寻找一个空闲的目录项或为其追加一个新的目录项;
	  需修改当前目录文件的长度信息，并将当前目录文件的用户打开文件表项中的fcbstate置为1；
	⑤ 准备好新建目录文件的FCB的内容，文件的属性为目录文件，
	  以覆盖写方式调用do_write()将其填写到对应的空目录项中；
	⑥ 在新建目录文件所分配到的磁盘块中建立两个特殊的目录项“.”和“..”目录项，
	  方法是：首先在用户空间中准备好内容，然后以截断写或者覆盖写方式调用do_write()将其写到③中分配到的磁盘块中；
	⑦ 返回。
	*/
	fcb *fcbptr;
	fat *fat1, *fat2;
	time_t now;
	struct tm *nowtime;
	char text[MAX_TEXT_SIZE];					// 存放打开表项中的内容
	unsigned short blkNum;
	int readByteNum, fd, i;
	// 定位到文件分配表起始位置
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	openfilelist[curdir].count = 0;				// 读写指针位置0
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);

	if(strchr(dirname, '.'))
	{
		printf("[Error]my_mkdir: dirname can't contain '.'!\n");
		return;
	}

	// 检查新建目录是否重名
	fcbptr = (fcb *)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if(strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
		{
			printf("[Error]my_mkdir: The dirname is already exists!\n");
			return;
		}
		fcbptr++;
	}
	// 找到空闲的打开表项
	fcbptr = (fcb*)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (fcbptr->free == 0)
			break;
		fcbptr++;
	}
	blkNum = getFreeBLOCK();
	if(blkNum == -1)
	{
		printf("[Error]my_mkdir: Can't find free blocks.\n");
		return;
	}
	(fat1 + blkNum)->id = END;
	(fat2 + blkNum)->id = END;
	// 写FCB信息
	now = time(NULL);
	nowtime = localtime(&now);
	strcpy(fcbptr->filename, dirname);
	strcpy(fcbptr->exname, "");
	fcbptr->attribute = 0;
	fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	fcbptr->first = blkNum;
	fcbptr->length = 2 * sizeof(fcb);
	fcbptr->free = 1;
	openfilelist[curdir].count = i * sizeof(fcb);
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);

	fd = my_open(dirname);						// 取得新建的目录的用户打开表项序号
	if (fd == -1)
		return;
	// 在新建的目录文件所分配到的磁盘块中建立目录项"."和".."
	fcbptr = (fcb *)malloc(sizeof(fcb));
	now = time(NULL);
	nowtime = localtime(&now);
	strcpy(fcbptr->filename, ".");
	strcpy(fcbptr->exname, "");
	fcbptr->attribute = 0;
	fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	fcbptr->first = blkNum;
	fcbptr->length = 2 * sizeof(fcb);
	fcbptr->free = 1;
	do_write(fd, (char *)fcbptr, sizeof(fcb), 2);

	now = time(NULL);
	nowtime = localtime(&now);
	strcpy(fcbptr->filename, "..");
	strcpy(fcbptr->exname, "");
	fcbptr->attribute = 0;
	fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	fcbptr->first = blkNum;
	fcbptr->length = 2 * sizeof(fcb);
	fcbptr->free = 1;
	do_write(fd, (char *)fcbptr, sizeof(fcb), 2);
	free(fcbptr);
	my_close(fd);

	fcbptr = (fcb*)text;
	fcbptr->length = openfilelist[curdir].length;
	openfilelist[curdir].count = 0;
	do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
	openfilelist[curdir].fcbstate = 1;
}

/*
my_rmdir | 删除子目录
功能:在当前目录下删除名为dirname的子目录。
输入:
	dirname:欲删除目录的目录名。
	① 调用do_read()读入当前目录文件内容到内存，检查当前目录下欲删除目录文件是否存在，
	  若不存在则返回，并显示错误信息；
	② 检查欲删除目录文件是否为空（除了“.”和“..”外没有其他子目录和文件），
	  可根据其目录项中记录的文件长度来判断，若不为空则返回，并显示错误信息；
	③ 检查该目录文件是否已经打开，若已打开则调用my_close()关闭掉；
	④ 回收该目录文件所占据的磁盘块，修改FAT；
	⑤ 从当前目录文件中清空该目录文件的目录项，且free字段置为0：以覆盖写方式调用do_write()来实现；
	⑥ 修改当前目录文件的用户打开表项中的长度信息，并将表项中的fcbstate置为1；
	⑦ 返回。
总结:
1. 把当前目录的目录文件读取到buf里
2. 在里面查找有没有匹配dirname的目录文件
3. 删除这个文件目录占用的所有盘块(fat全部释放，备份到fat2)
4. 更新当前目录文件，把dirname对应的fcb清除，更新当前目录文件大小
*/
void my_rmdir(char* dirname)
{
	fcb *fcbptr, *fcbptr2;
	fat *fat1, *fat2, *fatptr1, *fatptr2;
	char text[MAX_TEXT_SIZE], text2[MAX_TEXT_SIZE];
	unsigned short blkNum;
	int readByteNum, readByteNum2, fd, i, j;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	// 不能删除 . 和 ..
	if(strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
	{
		printf("[Error]my_rmdir: Can't remove this directory.\n");
		return;
	}
	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);
	fcbptr = (fcb*)text;
	// 寻找要删除的目录
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
			break;
		fcbptr++;
	}
	if (i == readByteNum / sizeof(fcb))
	{
		printf("[Error]my_rmdir: This directory does not exist.\n");
		return;
	}
	// 如果要删除的目录存在，检测是否为空
	fd = my_open(dirname);
	readByteNum2 = do_read(fd, openfilelist[fd].length, text2);
	fcbptr2 = (fcb*)text2;
	for (j = 0; j < readByteNum2 / sizeof(fcb); j++)
	{
		if(strcmp(fcbptr2->filename, ".") && strcmp(fcbptr2->filename, "..") &&
			strcmp(fcbptr2->filename, ""))
		{
			my_close(fd);
			printf("[Error]my_rmdir: This directory is not empty.\n");
			return;
		}
		fcbptr2++;
	}
	blkNum = openfilelist[fd].first;
	// 修改要删除目录在fat中所占用的目录项属性
	while (blkNum != END)
	{
		// 将所有占用盘块置为空闲
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;
		fatptr1->id = FREE;
		fatptr2->id = FREE;
	}
	my_close(fd);
	strcpy(fcbptr->filename, "");						// 修改已删除目录在当前目录的fcb属性
	fcbptr->free = 0;
	openfilelist[curdir].count = i * sizeof(fcb);		// 更新当前目录文件的内容
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
	openfilelist[curdir].fcbstate = 1;
}

/*
my_ls | 显示目录函数
功能: 显示当前目录的内容(子目录和文件信息)
*/
void my_ls()
{
	/*
	① 调用do_read()读出当前目录文件内容到内存；
	② 将读出的目录文件的信息按照一定的格式显示到屏幕上；
	③ 返回。
	*/
	fcb *fcbptr;
	char buf[MAX_TEXT_SIZE];
	int readByteNum, i;

	if(openfilelist[curdir].attribute == 1)			// 如果是数据文件
	{
		printf("[Error]my_ls: Can't use ls in data files.\n");
		return;
	}
	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, buf);
	fcbptr = (fcb*)buf;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (fcbptr->free)							// 如果目录项已分配
		{
			if (fcbptr->attribute == 0)				// 目录
				printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n",
					fcbptr->filename,
					(fcbptr->date >> 9) + 1980,
					(fcbptr->date >> 5) & 0x000f,
					fcbptr->date & 0x001f,
					fcbptr->time >> 11,
					(fcbptr->time >> 5) & 0x003f,
					fcbptr->time & 0x001f * 2);
			else									// 数据文件
				printf("%s.%s\t\t%dB\t\t%d/%d/%d\t%02d:%02d:%02d\t\n",
					fcbptr->filename,
					fcbptr->exname,
					(int)(fcbptr->length),
					(fcbptr->date >> 9) + 1980,
					(fcbptr->date >> 5) & 0x000f,
					fcbptr->date & 0x1f,
					fcbptr->time >> 11,
					(fcbptr->time >> 5) & 0x3f,
					fcbptr->time & 0x1f * 2);
		}
		fcbptr++;
	}
}

/*
my_create | 创建文件函数
功能:创建名为filename的新文件。
输入:
	filename:新建文件的文件名，可能包含路径。
输出:若创建成功，返回该文件的文件描述符（文件打开表中的数组下标）；否则返回-1。
① 为新文件分配一个空闲打开文件表项，如果没有空闲表项则返回-1，并显示错误信息；
② 若新文件的父目录文件还没有打开，则调用my_open()打开；
  若打开失败，则释放①中为新建文件分配的空闲文件打开表项，返回-1，并显示错误信息；
③ 调用do_read()读出该父目录文件内容到内存，检查该目录下新文件是否重名，若重名则释放①中分配的打开文件表项，并调用my_close()关闭②中打开的目录文件；然后返回-1，并显示错误信息；
④ 检查FAT是否有空闲的盘块，如有则为新文件分配一个盘块，否则释放①中分配的打开文件表项，并调用my_close()关闭②中打开的目录文件；返回-1，并显示错误信息；
⑤ 在父目录中为新文件寻找一个空闲的目录项或为其追加一个新的目录项;需修改该目录文件的长度信息，并将该目录文件的用户打开文件表项中的fcbstate置为1；
⑥ 准备好新文件的FCB的内容，文件的属性为数据文件，长度为0，以覆盖写方式调用do_write()将其填写到⑤中分配到的空目录项中；
⑦ 为新文件填写①中分配到的空闲打开文件表项，fcbstate字段值为0，读写指针值为0；
⑧ 调用my_close()关闭②中打开的父目录文件；
⑨ 将新文件的打开文件表项序号作为其文件描述符返回。
*/
void my_create(char* filename)
{
	fcb *fcbptr;
	fat *fat1, *fat2;
	char *fname, *exname, text[MAX_TEXT_SIZE];
	unsigned short blkNum;
	int readByteNum, i;
	time_t now;
	struct tm *nowtime;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	fname = strtok(filename, ".");
	exname = strtok(NULL, ".");
	if(strcmp(fname, "") == 0)
	{
		printf("[Error]my_create: File must have a name.\n");
		return;
	}
	if(!exname)
	{
		printf("[Error]my_create: File must have a extern name.\n");
		return;
	}
	openfilelist[curdir].count = 0;
	// 读取当前目录下的文件信息，检测文件是否已经存在
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);
	fcbptr = (fcb*)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if(strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
		{
			printf("[Error]my_create: This file is already exist.\n");
			return;
		}
		fcbptr++;
	}
	// 获取空闲fcb和盘块
	fcbptr = (fcb*)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (0 == fcbptr->free)
			break;
		fcbptr++;
	}
	blkNum = getFreeBLOCK();
	(fat1 + blkNum)->id = END;
	(fat2 + blkNum)->id = END;
	// 属性赋值
	now = time(NULL);
	nowtime = localtime(&now);
	strcpy(fcbptr->filename, fname);
	strcpy(fcbptr->exname, exname);
	fcbptr->attribute = 1;
	fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	fcbptr->first = blkNum;
	fcbptr->length = 0;
	fcbptr->free = 1;
	openfilelist[curdir].count = i * sizeof(fcb);
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);			// 把新建文件的目录项写入到当前目录文件中

	fcbptr = (fcb *)text;
	fcbptr->length = openfilelist[curdir].length;
	openfilelist[curdir].count = 0;
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);			// 修改当前目录的长度
	openfilelist[curdir].fcbstate = 1;
}

/*
my_rm | 删除文件函数
功能:删除名为filename的文件。
输入:
	filename:欲删除文件的文件名，可能还包含路径。
	① 若欲删除文件的父目录文件还没有打开，则调用my_open()打开；若打开失败，则返回，并显示错误信息；
	② 调用do_read()读出该父目录文件内容到内存，检查该目录下欲删除文件是否存在，若不存在则返回，并显示错误信息；
	③ 检查该文件是否已经打开，若已打开则关闭掉；
	④ 回收该文件所占据的磁盘块，修改FAT；
	⑤ 从文件的父目录文件中清空该文件的目录项，且free字段置为0：以覆盖写方式调用do_write()来实现；；
	⑥ 修改该父目录文件的用户打开文件表项中的长度信息，并将该表项中的fcbstate置为1；
	⑦ 返回。
总结:
1. 读取当前目录的目录文件到buf
2. 在buf中寻找匹配filename的数据文件
3. 清空这个文件占据的盘块(释放fat，并且备份)
4. 在父目录文件中，删除filename对应的fcb
5. 更新父目录文件的长度
*/
void my_rm(char* filename)
{
	fcb *fcbptr;
	fat *fat1, *fat2, *fatptr1, *fatptr2;
	char *fname, *exname, text[MAX_TEXT_SIZE];
	unsigned short blkNum;
	int readByteNum, i;

	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	fname = strtok(filename, ".");
	exname = strtok(NULL, ".");
	if(strcmp(fname, "") == 0)
	{
		printf("[Error]my_rm: Must have a file name.\n");
		return;
	}
	if(!exname)
	{
		printf("[Error]my_rm: Must have a extern name.\n");
		return;
	}

	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);		// 读取当前目录
	// 寻找匹配的文件
	fcbptr = (fcb*)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
			break;
		fcbptr++;
	}
	// 如果不存在
	if(i == readByteNum / sizeof(fcb))
	{
		printf("[Error]my_rm: This file does not exist.\n");
		return;
	}
	// 如果存在
	blkNum = fcbptr->first;
	while (blkNum != END)									// 处理该文件占用的所有磁盘块
	{
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;
		fatptr1->id = FREE;
		fatptr2->id = FREE;
	}
	// FCB归零
	strcpy(fcbptr->filename, "");
	fcbptr->free = 0;
	openfilelist[curdir].count = i * sizeof(fcb);
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
	openfilelist[curdir].fcbstate = 1;
}

/*
my_open | 打开文件函数
功能:打开当前目录下名为filename的文件。
输入:
	filename:欲打开文件的文件名
输出:若打开成功，返回该文件的描述符（在用户打开文件表中表项序号）；否则返回-1。
	① 检查该文件是否已经打开，若已打开则返回-1，并显示错误信息；
	② 调用do_read()读出父目录文件的内容到内存，检查该目录下欲打开文件是否存在，
	  若不存在则返回-1，并显示错误信息；
	③ 检查用户打开文件表中是否有空表项，若有则为欲打开文件分配一个空表项，
	  若没有则返回-1，并显示错误信息；
	④ 为该文件填写空白用户打开文件表表项内容，读写指针置为0；
	⑤ 将该文件所分配到的空白用户打开文件表表项序号（数组下标）作为文件描述符fd返回。
总结:
把当前目录文件读取到buf里,buf里就是一个个fcb，在这些fcb中寻找filename匹配的文件
然后从openfilelist里取一个项，把这个数据文件写进去，切换currfd=fd，就算打开文件
*/
int my_open(char* filename)
{
	fcb *fcbptr;
	char *fname, exname[EXNAME_SIZE], *str, text[MAX_TEXT_SIZE];
	int readByteNum, fd, i;

	fname = strtok(filename, ".");					// 分出文件名
	str = strtok(NULL, ".");						// 分出扩展名
	
	if (str)										// 如果有扩展名
		strcpy(exname, str);
	else											// 如果没有扩展名
		strcpy(exname, "");
	// 在用户打开文件数组查找当前文件是否已经打开
	for (i = 0; i < MAXOPENFILE; i++)
	{
		if(strcmp(openfilelist[i].filename, fname) == 0 && strcmp(openfilelist[i].exname, exname) == 0)
		{
			printf("[Error]my_open: This file is already open.\n");
			return -1;
		}
	}
	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);
	fcbptr = (fcb*)text;
	// 寻找文件,找到定位fcbptr
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
			break;
		fcbptr++;
	}
	// 没有找到文件
	if (i == readByteNum / sizeof(fcb))
	{
		printf("[Error]my_open: This file doesn't exist.\n");
		return -1;
	}
	fd = getFreeOpenfilelist();						// 寻找空闲文件表项
	if (fd == -1)
	{
		printf("[Error]my_open: Can't find free openfilelist item.\n");
		return -1;
	}

	strcpy(openfilelist[fd].filename, fcbptr->filename);
	strcpy(openfilelist[fd].exname, fcbptr->exname);
	openfilelist[fd].attribute = fcbptr->attribute;
	openfilelist[fd].time = fcbptr->time;
	openfilelist[fd].date = fcbptr->date;
	openfilelist[fd].first = fcbptr->first;
	openfilelist[fd].length = fcbptr->length;
	openfilelist[fd].free = fcbptr->free;
	openfilelist[fd].dirno = openfilelist[curdir].first;
	openfilelist[fd].diroff = i;
	//文件目录名; 
	strcpy(openfilelist[fd].dir, openfilelist[curdir].dir);
	strcat(openfilelist[fd].dir, filename);
	if (fcbptr->attribute & 0x20)
		strcat(openfilelist[fd].dir, "\\");
	openfilelist[fd].father = curdir;
	openfilelist[fd].count = 0;
	openfilelist[fd].fcbstate = 0;					//未修改FCB; 
	openfilelist[fd].topenfile = 1;					//打开表项状态置为已占用; 
	return fd;
}

/*
my_close | 关闭文件函数
功能:关闭前面由my_open()打开的文件描述符为fd的文件。
输入:
	fd:文件描述符。

	① 检查fd的有效性（fd不能超出用户打开文件表所在数组的最大下标），如果无效则返回-1；
	② 检查用户打开文件表表项中的fcbstate字段的值，如果为1则需要将该文件的FCB的内容保存到虚拟磁盘上该文件的目录项中，方法是：打开该文件的父目录文件，以覆盖写方式调用do_write()将欲关闭文件的FCB写入父目录文件的相应盘块中；
	③ 回收该文件占据的用户打开文件表表项（进行清空操作），并将topenfile字段置为0；
	④ 返回。
*/
int my_close(int fd)
{
	fcb *fcbptr;
	int father;
	if (fd < 0 || fd >= MAXOPENFILE)
	{
		printf("[Error]my_close: This file does not exist.\n");
		return -1;
	}
	/**
	如果文件fcb被修改过，要写回去
	 *先把父目录文件从磁盘中读取到buf中，然后把更新后的fcb内容写到buf里，然后再从buf写到磁盘
	 *写回磁盘时，只要让count指向这个文件对应的fcb位置，然后写入fcb即可
	 */
	if (openfilelist[fd].fcbstate)
	{
		fcbptr = (fcb*)malloc(sizeof(fcb));

		strcpy(fcbptr->filename, openfilelist[fd].filename);
		strcpy(fcbptr->exname, openfilelist[fd].exname);
		fcbptr->attribute = openfilelist[fd].attribute;
		fcbptr->time = openfilelist[fd].time;
		fcbptr->date = openfilelist[fd].date;
		fcbptr->first = openfilelist[fd].first;
		fcbptr->length = openfilelist[fd].length;
		fcbptr->free = openfilelist[fd].free;
		father = openfilelist[fd].father;
		//diroff为相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号; 
		openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);
		//以覆盖写方式将欲关闭的文件的FCB写入父目录文件的相应盘块中; 
		do_write(father, (char *)fcbptr, sizeof(fcb), 2);
		//回收; 
		free(fcbptr);
		openfilelist[fd].fcbstate = 0;
	}
	//回收打开表项; 
	strcpy(openfilelist[fd].filename, "");
	strcpy(openfilelist[fd].exname, "");
	openfilelist[fd].topenfile = 0;
	father = openfilelist[fd].father;
	return father;//返回父路径的文件描述符(打开表项序号); 
}

/*
my_write | 写文件函数
功能：将用户通过键盘输入的内容写到fd所指定的文件中。
输入:
	fd:		open()函数的返回值，文件的描述符；
输出：实际写入的字节数。

1.截断写:	放弃原来文件的内容，重新写文件
2.覆盖写:	修改文件在当前读写指针所指的位置开始的部分内容；
3.追加写:	是在原文件的最后添加新的内容。
截断写:释放文件所在的除第一块外的其它盘块(需要时再分配)并将读写指针置0; 
覆盖写:直接; 
追加写:读写指针放到文件末尾;
*/
int my_write(int fd)
{
	fat *fat1, *fat2, *fatptr1, *fatptr2;
	int wstyle, len, ll, tmp;
	char text[MAX_TEXT_SIZE];
	unsigned short blkNum;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	if (fd < 0 || fd >= MAXOPENFILE)
	{
		printf("[Error]my_write: File does not exist.\n");
		return -1;
	}
	while(1)
	{
		//1.截断写 2.覆盖写 3.追加写
		printf("Enter write style:\n1. Cut write\t2. Cover write\t3. Add write\n");
		scanf("%d", &wstyle);
		if (wstyle > 0 && wstyle < 4)
			break;
		printf("[Error]my_write: Write style error!\n");
	}
	getchar();
	switch (wstyle)
	{
	case 1:								// 截断写，把源文件所占的虚拟磁盘空间重置为1
		blkNum = openfilelist[fd].first;
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;			// 获取fat表中第二块块号
		fatptr1->id = END;				// 第一块独立
		fatptr2->id = END;
		// 如果有下一块
		while (blkNum != END)
		{
			fatptr1 = fat1 + blkNum;
			fatptr2 = fat2 + blkNum;
			blkNum = fatptr1->id;
			fatptr1->id = FREE;
			fatptr2->id = FREE;
		}
		// 读写指针和文件长度置0
		openfilelist[fd].count = 0;
		openfilelist[fd].length = 0;
		break;
	case 2:								// 覆盖写
		openfilelist[fd].count = 0;
		break;
	case 3:								// 追加写
		openfilelist[fd].count = openfilelist[fd].length;
		break;
	default:
		break;
	}
	ll = 0;
	printf("Input data (end with Ctrl+Z):\n");
	while (gets_s(text))
	{
		len = strlen(text);
		text[len++] = '\n';
		text[len] = '\0';
		tmp = do_write(fd, text, len, wstyle);
		if (-1 != tmp)
			ll += tmp;
		if(tmp < len)
		{
			printf("[Error]my_write: Write error!\n");
			break;
		}
	}
	return ll;
}

int main()
{
	char cmd[15][10] = {
		"cd", "mkdir", "rmdir", "ls", "create", "rm", "open", "close", "write", "read", "exit"
	};
	char s[30], *sp;
	int cmdn, flag = 1, i;
	my_startsys();
	printf("*********************Scarb文件系统 V0.0*******************************\n\n");
	printf("命令名\t\t命令参数\t\t命令说明\n\n");
	printf("cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n");
	printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
	printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
	printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
	printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
	printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
	printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
	printf("write\t\t无\t\t\t在打开文件状态下，写该文件\n");
	printf("read\t\t无\t\t\t在打开文件状态下，读取该文件\n");
	printf("close\t\t无\t\t\t在打开文件状态下，关闭该文件\n");
	printf("exit\t\t无\t\t\t退出系统\n\n");
	printf("*********************************************************************\n\n");
	while(flag)
	{
		printf("%s>", openfilelist[curdir].dir);
		gets_s(s);		// gets()
		cmdn = -1;
		if(strcmp(s, ""))
		{
			sp = strtok(s, " ");				// 根据空格分割字符串
			for (i = 0; i < 15; i++)
			{
				if(strcmp(sp, cmd[i]) == 0)
				{
					cmdn = i;
					break;
				}
			}
			switch (cmdn)
			{
			case 0:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_cd(sp);
				else
					printf("请输入正确的指令.\n");
				break;
			case 1:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_mkdir(sp);
				else
					printf("请输入正确的指令.\n");
				break;
			case 2:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_rmdir(sp);
				else
					printf("请输入正确的指令.\n");
				break;
			case 3:
				if (openfilelist[curdir].attribute == 0)
					my_ls();
				else
					printf("请输入正确的指令.\n");
				break;
			case 4:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_create(sp);
				else
					printf("请输入正确的指令.\n");
				break;
			case 5:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_rm(sp);
				else
					printf("请输入正确的指令.\n");
				break;
			case 6:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
				{
					if (strchr(sp, '.'))//查找sp中'.'首次出现的位置
						curdir = my_open(sp);
					else
						printf("打开的文件没有后缀名.\n");
				}
				else
					printf("请输入正确的指令.\n");
				break;
			case 7:
				if (!(openfilelist[curdir].attribute == 0))
					curdir = my_close(curdir);
				else
					printf("文件打开失败.\n");
				break;
			case 8:
				if (!(openfilelist[curdir].attribute == 0))
					my_write(curdir);
				else
					printf("文件打开失败.\n");
				break;
			case 9:
				if (!(openfilelist[curdir].attribute == 0))
					my_read(curdir, openfilelist[curdir].length);
				else
					printf("文件打开失败.\n");
				break;
			case 10:
				if (openfilelist[curdir].attribute == 0)
				{
					my_exitsys();
					flag = 0;
				}
				else
					printf("请输入正确的指令.\n");
				break;
			default:
				printf("请输入正确的指令.\n");
				break;
			}
		}
	}
}