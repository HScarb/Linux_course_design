#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* define const */
#define BLOCKSIZE           1024    // ���̿��С
#define SIZE                1024000 // ������̿ռ��С
#define END                 65535   // FAT�е��ļ�������־
#define FREE                0       // FAT���̿���б�־
#define ROOTBLOCKNUM		2		// ��Ŀ¼��ռ�̿���
#define MAXOPENFILE         10      // ���ͬʱ�򿪵��ļ�����

#define MAX_TEXT_SIZE       10000
#define FILENAME_SIZE		8		// �ļ�������
#define EXNAME_SIZE			4		// �ļ���չ������

/* define data structures */
/**
* File Control Block |���ļ����ƿ� | ����FAT16
���ڼ�¼�ļ��������Ϳ�����Ϣ��ÿ���ļ�����һ��FCB����Ҳ���ļ���Ŀ¼������ݡ�
*/
typedef struct FCB {
	char filename[FILENAME_SIZE];   // �ļ���
	char exname[EXNAME_SIZE];		// �ļ���չ��
	unsigned char attribute;        // �ļ������ֶΣ�ֵΪ0ʱ��ʾĿ¼�ļ���ֵΪ1ʱ��ʾ�����ļ�
	unsigned short time;            // �ļ�����ʱ��
	unsigned short date;            // �ļ���������
	unsigned short first;           // �ļ���ʼ�̿��
	unsigned long length;           // �ļ�����(�ֽ���)
	char free;                      // ��ʾĿ¼���Ƿ�Ϊ�գ�0��ʾ�գ�1��ʾ�ѷ���
} fcb;

/**
* FAT : File Allocation Table
* �ļ������
�ڱ�ʵ���У��ļ���������������ã�һ�Ǽ�¼������ÿ���ļ���ռ�ݵĴ��̿�Ŀ�ţ�
	���Ǽ�¼��������Щ���Ѿ������ȥ�ˣ���Щ���ǿ��еģ�������λʾͼ�����á�
	��FAT��ĳ�������ֵΪFREE�����ʾ�ñ�������Ӧ�Ĵ��̿��ǿ��еģ�
	��ĳ�������ֵΪEND�����ʾ����Ӧ�Ĵ��̿���ĳ�ļ������һ�����̿飻
	��ĳ�������ֵ������ֵ�����ֵ��ʾĳ�ļ�����һ�����̿�Ŀ�š�
	Ϊ�����ϵͳ�Ŀɿ��ԣ���ʵ��������������FAT�����ǻ�Ϊ���ݣ�ÿ��FATռ���������̿顣
*/
typedef struct FAT {
	unsigned short id;				// ���̿��״̬(���еģ����ģ���һ��)
} fat;

/**	�û����ļ���
����һ���ļ�ʱ�����뽫�ļ���Ŀ¼���е���������ȫ�����Ƶ��ڴ��У�
ͬʱ��Ҫ��¼�й��ļ������Ķ�̬��Ϣ�����дָ���ֵ�ȡ�
�ڱ�ʵ����ʵ�ֵ���һ�����ڵ��û�������ϵͳ���ļ�ϵͳ��
Ϊ����������ǰ��û��ļ�����������ڴ�FCB�����һ�𣬳�Ϊ�û����ļ���
������ĿΪ10����һ���û�����ͬʱ��10���ļ���
Ȼ����һ���������������������±꼴ĳ�����ļ�����������
���⣬�������û����ļ����л�������һ���ֶΡ�char dir[80]����
������¼ÿ�����ļ����ڵ�Ŀ¼�����Է����û��򿪲�ͬĿ¼�¾�����ͬ�ļ����Ĳ�ͬ�ļ���
 */
typedef struct USEROPEN {
	char filename[FILENAME_SIZE];	// �ļ���
	char exname[EXNAME_SIZE];		// �ļ���չ��
	unsigned char attribute;        // �ļ������ֶΣ�ֵΪ0ʱ��ʾĿ¼�ļ���ֵΪ1ʱ��ʾ�����ļ�
	unsigned short time;            // �ļ�����ʱ��
	unsigned short date;            // �ļ���������
	unsigned short first;           // �ļ���ʼ�̿��
	unsigned long length;           // �ļ����ȣ��������ļ����ֽ�������Ŀ¼�ļ�������Ŀ¼�������
	char free;                      // ��ʾĿ¼���Ƿ�Ϊ�գ���ֵΪ0����ʾ�գ�ֵΪ1����ʾ�ѷ���
	// �������õ�dirno��diroff��¼����Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ��е�λ�ã�
	// ����������ļ���fcb���޸��ˣ���Ҫд�ظ�Ŀ¼�ļ�ʱ�ȽϷ���
	int dirno;                      // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ��е��̿��
	int diroff;                     // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ���dirno�̿��е�Ŀ¼�����
	char dir[80];                   // ��Ӧ���ļ����ڵ�Ŀ¼��������������ټ���ָ���ļ��Ƿ��Ѿ���
	int father;						// ��Ŀ¼�ڴ��ļ������λ��
	int count;                      // ��дָ�����ļ��е�λ��
	char fcbstate;                  // �Ƿ��޸����ļ���FCB�����ݣ�����޸�����Ϊ1������Ϊ0
	char topenfile;                 // ��ʾ���û��򿪱����Ƿ�Ϊ�գ���ֵΪ0����ʾΪ�գ������ʾ�ѱ�ĳ���ļ�ռ��
} useropen;

/**
* ������
������������Ҫ����߼����̵����������Ϣ��
������̿��С�����̿��������ļ��������Ŀ¼�����������ڴ����ϵ���ʼλ�õȡ�
����������̣���Ҫ��Ų���ϵͳ��������Ϣ��
��ʵ�������ڴ����������д���һ���ļ�ϵͳ����������������ݱȽ��٣�
ֻ�д��̿��С�����̿���������������ʼλ�á���Ŀ¼�ļ���ʼλ�õȡ�
*/
typedef struct BLOCK0 {
	char magic_number[8];           // �ļ�ϵͳ��ħ��
	char information[200];
	unsigned short root;            // ��Ŀ¼�ļ�����ʼ�̿��
	unsigned char * startblock;     // �����̿�����������ʼλ��
}block0;

/* global variables */
unsigned char * myvhard;            // ָ��������̵���ʼ��ַ / unsigned char = byte
useropen openfilelist[MAXOPENFILE]; // �û����ļ�������
//int currfd;                         // current field/��¼��ǰ�û����ļ������±�
int curdir;							// �û����ļ����еĵ�ǰĿ¼���ڴ��ļ������λ��(�±�)
char currentdir[80];				// ��¼��ǰĿ¼��Ŀ¼��(����Ŀ¼��·��)
unsigned char * startp;             // ��¼�����������������ʼλ��
char * myFileName = "myfilesys.txt";  // ���ݴ洢·��
unsigned char buffer[SIZE];         // ������

/* functions */

void my_startsys();                 // �����ļ�ϵͳ
void my_exitsys();                  // �˳��ļ�ϵͳ
void my_format();                   // ���̸�ʽ������
void my_mkdir(char *dirname);       // ������Ŀ¼
void my_rmdir(char *dirname);       // ɾ����Ŀ¼
void my_ls();                       // ��ʾĿ¼�е�����
void my_cd(char *dirname);          // ���ڸ��ĵ�ǰĿ¼
void  my_create(char *filename);     // �����ļ�
void my_rm(char *filename);         // ɾ���ļ�
int  my_open(char *filename);       // ���ļ�
int  my_close(int fd);              // �ر��ļ�
int  my_write(int fd);              // д�ļ�
int  my_read(int fd, int len);      // ���ļ�
int  do_write(int fd, char *text, int len, char wstyle);
int  do_read(int fd, int len, char *text);

unsigned short getFreeBLOCK();      // ��ȡһ�����еĴ��̿�
int getFreeOpenfilelist();          // ��ȡһ�����е��ļ��򿪱���
//int find_father_dir(int fd);        // Ѱ��һ�����ļ��ĸ�Ŀ¼���ļ�

unsigned short getFreeBLOCK()
{
	unsigned short i;
	fat *fat1, *fatptr;
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	for (i = 7; i < SIZE / BLOCKSIZE; i++)		// i = 7,��������7��ʼ
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
my_startsys | �����ļ�ϵͳ����
1. Ѱ��myfsys.txt�ļ���������ڶ��ҿ�ͷ���ļ�ħ�����Ͷ�ȡ��myvhard��
	  ���򴴽��ļ���д���ʼ����Ϣ
2. �����û����ļ���ĵ�һ���������Ϊ��Ŀ¼���ݣ���Ĭ�ϴ򿪸�Ŀ¼
3. ��ʼ��һ��ȫ�ֱ���
*/
void my_startsys()
{
	/**
	�� ����������̿ռ䣻
	�� ʹ��c���ԵĿ⺯��fopen()��myfsys�ļ������ļ����ڣ���ת�ۣ����ļ������ڣ��򴴽�֮��ת��
	�� ʹ��c���ԵĿ⺯��fread()����myfsys�ļ����ݵ��û��ռ��е�һ���������У�
	���ж��俪ʼ��8���ֽ������Ƿ�Ϊ��10101010�����ļ�ϵͳħ����������ǣ���ת�ܣ�����ת�ݣ�
	�� �������������е����ݸ��Ƶ��ڴ��е�������̿ռ��У�ת��
	�� ����Ļ����ʾ��myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ����Ϣ��
	������my_format()�Ԣ������뵽��������̿ռ���и�ʽ��������ת�ޣ�
	�� ����������е����ݱ��浽myfsys�ļ��У�ת��
	�� ʹ��c���ԵĿ⺯��fclose()�ر�myfsys�ļ���
	�� ��ʼ���û����ļ���������0�������Ŀ¼�ļ�ʹ�ã�����д��Ŀ¼�ļ��������Ϣ��
	���ڸ�Ŀ¼û���ϼ�Ŀ¼�����Ա����е�dirno��diroff�ֱ���Ϊ5����Ŀ¼������ʼ��ţ���0��
	����ptrcurdirָ��ָ����û����ļ����
	�� ����ǰĿ¼����Ϊ��Ŀ¼��
	*/
	FILE * file;
	int i;
	myvhard = (unsigned char *)malloc(SIZE);
	// ����ļ������ڻ��߿�ͷ�����ļ�ħ�������´����ļ�
	if ((file = fopen(myFileName, "r")) != NULL)
	{
		fread(buffer, SIZE, 1, file);			// ���������ļ���ȡ��������
		fclose(file);
		if(memcmp(buffer, "10101010", 8) == 0)
		{
			memcpy(myvhard, buffer, SIZE);
			printf("myfsys�ļ���ȡ�ɹ�\n");
		}
		else									// ���ļ����ǿ�ͷ�����ļ�ħ��
		{
			printf("myfsys�ļ�ϵͳ�����ڣ����ڴ����ļ�ϵͳ\n");
			my_format();
			memcpy(buffer, myvhard, SIZE);
		}
	}
	else
	{
		printf("myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ\n");
		my_format();
		memcpy(buffer, myvhard, SIZE);
	}

	/**
	 *	�� ��ʼ���û����ļ���������0�������Ŀ¼�ļ�ʹ�ã�����д��Ŀ¼�ļ��������Ϣ��
	 *	  ���ڸ�Ŀ¼û���ϼ�Ŀ¼�����Ա����е�dirno��diroff�ֱ���Ϊ5����Ŀ¼������ʼ��ţ���0��
	 *	  ����ptrcurdirָ��ָ����û����ļ����
	 *  �� ����ǰĿ¼����Ϊ��Ŀ¼��
	 */
	fcb *root;
	root = (fcb*)(myvhard + 5 * BLOCKSIZE);		// ��Ŀ¼�׵�ַ
	strcpy(openfilelist[0].filename, root->filename);
	strcpy(openfilelist[0].exname, root->exname);
	openfilelist[0].attribute = root->attribute;
	openfilelist[0].time = root->time;
	openfilelist[0].date = root->date;
	openfilelist[0].first = root->first;
	openfilelist[0].length = root->length;
	openfilelist[0].free = root->free;
	openfilelist[0].dirno = 5;					// ��6��
	openfilelist[0].diroff = 0;
	strcpy(openfilelist[0].dir, "root\\");		// ���ļ����ڵ�Ŀ¼��
	openfilelist[0].father = 0;
	openfilelist[0].count = 0;					// ��дָ���λ��
	openfilelist[0].fcbstate = 0;
	openfilelist[0].topenfile = 1;				// ��򿪱���
	for (i = 1; i < MAXOPENFILE; i++)			// ����������Ϊ�Ǵ�״̬
		openfilelist[i].topenfile = 0;
	
	curdir = 0;									// ��ǰ����
	strcpy(currentdir, "root\\");				// ��ǰĿ¼��
	startp = ((block0 *)myvhard)->startblock;	// ��������ʼλ��
}

/*
my_exitsts | �˳��ļ�ϵͳ����
*/
void my_exitsys()
{
	/*
	�� ʹ��C�⺯��fopen()�򿪴����ϵ�myfsys�ļ���
	�� ��������̿ռ��е��������ݱ��浽�����ϵ�myfsys�ļ��У�
	�� ʹ��c���ԵĿ⺯��fclose()�ر�myfsys�ļ���
	�� �����û����ļ����ͷ����ڴ�ռ�
	�� �ͷ�������̿ռ䡣
	*/
	FILE *file;
	while (curdir)
		curdir = my_close(curdir);			// �ر����д򿪵�Ŀ¼
	file = fopen(myFileName, "w");
	fwrite(myvhard, SIZE, 1, file);
	fclose(file);
	free(myvhard);
}


/*
my_format | ���̸�ʽ������
1. ����������(һ���̿�)
2. ����FAT1(�����̿�) FAT2(�����̿�)
3. ���ø�Ŀ¼�ļ�����������Ŀ¼��.��..(��Ŀ¼�ļ�ռһ���̿飬����Ŀ¼��д������̿�����)
*/
void my_format()
{
	/**
	�� ��������̵�һ������Ϊ�����飬��ʼ��8���ֽ����ļ�ϵͳ��ħ������Ϊ��10101010����
	��֮��д���ļ�ϵͳ��������Ϣ����FAT���С��λ�á���Ŀ¼��С��λ�á��̿��С���̿���������������ʼλ�õ���Ϣ��
	�� �����������������ȫһ����FAT�����ڼ�¼�ļ���ռ�ݵĴ��̿鼰����������̿�ķ��䣬
	ÿ��FATռ���������̿飻����ÿ��FAT�У�ǰ��5��������Ϊ�ѷ��䣬����995��������Ϊ���У�
	�� �ڵڶ���FAT�󴴽���Ŀ¼�ļ�root�����������ĵ�1�飨��������̵ĵ�6�飩�������Ŀ¼�ļ���
	�ڸô����ϴ������������Ŀ¼���.���͡�..���������ݳ����ļ�����֮ͬ�⣬�����ֶ���ȫ��ͬ��
	*/
	FILE * file;
	fat *fat1, *fat2;
	block0 *boot;
	time_t now;
	struct tm * nowtime;
	fcb *root;
	int i;

	boot = (block0 *)myvhard;					// ������λ�ڴ����ײ�
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	root = (fcb*)(myvhard + 5 * BLOCKSIZE);
	strcpy(boot->magic_number, "10101010");
	strcpy(boot->information, "Scarb�ļ�ϵͳ,�����䷽ʽ:FAT; ���̿ռ����:�����FAT��λʾͼ; �ṹ:���û��༶Ŀ¼�ṹ");
	boot->root = 5;
	boot->startblock = (unsigned char *)root;
	for (i = 0; i < 5; i++)
	{
		fat1->id = END;
		fat2->id = END;
		fat1++;
		fat2++;
	}
	// ��Ŀ¼���ڵ�6��7��
	fat1->id = 6;
	fat2->id = 6;
	fat1++, fat2++;
	fat1->id = END;
	fat2->id = END;
	fat1++, fat2++;
	// ���������̿����
	for (i = 7; i < SIZE / BLOCKSIZE; i++)
	{
		fat1->id = FREE;
		fat2->id = FREE;
		fat1++, fat2++;
	}
	now = time(NULL);							// ����һ��ʱ���
	nowtime = localtime(&now);					// ת��Ϊ����ʱ��
	// ����Ŀ¼"."
	strcpy(root->filename, ".");
	strcpy(root->exname, "");
	root->attribute = 0;						// 0,Ŀ¼�ļ�
	root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
	root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
	root->first = 5;
	root->length = 2 * sizeof(fcb);
	root->free = 1;
	// ����Ŀ¼".."
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

	// д�ش���
	file = fopen(myFileName, "w");
	fwrite(myvhard, SIZE, 1, file);
	fclose(file);
}

/*
my_cd | ���ĵ�ǰĿ¼����
�� ����my_open()��ָ��Ŀ¼���ĸ�Ŀ¼�ļ���������do_read()����ø�Ŀ¼�ļ����ݵ��ڴ��У�
�� �ڸ�Ŀ¼�ļ��м���µĵ�ǰĿ¼���Ƿ���ڣ����������ת�ۣ����򷵻أ�����ʾ������Ϣ��
�� ����my_close()�رբ��д򿪵ĸ�Ŀ¼�ļ���
�� ����my_close()�ر�ԭ��ǰĿ¼�ļ���
�� ����µĵ�ǰĿ¼�ļ�û�д򿪣���򿪸�Ŀ¼�ļ�������ptrcurdirָ��ô��ļ����
�� ���õ�ǰĿ¼Ϊ��Ŀ¼��
�ܽ�:
�����ǰ��Ŀ¼�ļ���,��ô��Ҫ�����Ŀ¼�ļ���ȡ��buf��, Ȼ���������ļ����fcb��û��ƥ��dirname��Ŀ¼��(���ұ�����Ŀ¼�ļ�)
�����,�Ǿ���openfilelist��ȡһ�����ļ�����,�����dirname���Ŀ¼�ļ���fcbд��ȥ,Ȼ���л�currfd=fd
���������Ǵ�һ��Ŀ¼
*/
void my_cd(char* dirname)
{
	char *dir;
	int fd;
	dir = strtok(dirname, "\\");
	if (strcmp(dir, ".") == 0)					// ���ת����ǰĿ¼����������
		return;									
	else if(strcmp(dir, "..") == 0)				// ���ת���ϲ�Ŀ¼
	{
		if (curdir)
			curdir = my_close(curdir);			// �رյ�ǰĿ¼������ֵΪ��Ŀ¼�ڴ򿪱��λ��
		return;
	}
	else if(strcmp(dir, "root") == 0)
	{
		while (curdir)							// ѭ���ر�root�´򿪵��ļ�
		{
			curdir = my_close(curdir);
		}
		dir = strtok(NULL, "\\");
	}
	while (dir)
	{
		fd = my_open(dir);
		if (-1 != fd)							// �ɹ�ת��·��
			curdir = fd;						// ���õ�ǰĿ¼Ϊ�򿪵�Ŀ¼
		else
		{
			printf("[Error]my_cd: Can not cd this dir.\n");
			return;
		}
		dir = strtok(NULL, "\\");				// ʣ�µ�·��
	}
}

/*
do_read | ʵ�ʶ�ȡ�ļ�����
���ܣ���my_read()���ã�����ָ���ļ��дӶ�дָ�뿪ʼ�ĳ���Ϊlen�����ݵ��û��ռ��text�С�
���룺
	fd��		open()�����ķ���ֵ���ļ�����������
	len:	Ҫ����ļ��ж������ֽ�����
	text:	ָ���Ŷ������ݵ��û�����ַ
�����ʵ�ʶ������ֽ�����
*/
int do_read(int fd, int len, char* text)
{
	/*
	�� ʹ��malloc()����1024B�ռ���Ϊ������buf������ʧ���򷵻�-1������ʾ������Ϣ��
	�� ����дָ��ת��Ϊ�߼����ż�����ƫ����off��
	  ���ô��ļ�������е��׿�Ų���FAT���ҵ����߼������ڵĴ��̿��ţ�
	  ���ô��̿���ת��Ϊ��������ϵ��ڴ�λ�ã�
	�� �����ڴ�λ�ÿ�ʼ��1024B��һ�����̿飩���ݶ���buf�У�
	�� �Ƚ�buf�д�ƫ����off��ʼ��ʣ���ֽ����Ƿ���ڵ���Ӧ��д���ֽ���len��
	  ����ǣ��򽫴�off��ʼ��buf�е�len���ȵ����ݶ��뵽text[]�У�
	  ���򣬽���off��ʼ��buf�е�ʣ�����ݶ��뵽text[]�У�
	�� ����дָ�����Ӣ����Ѷ��ֽ�������Ӧ��д���ֽ���len��ȥ�����Ѷ��ֽ�������len����0����ת�ڣ�����ת�ޣ�
	�� ʹ��free()�ͷŢ��������buf��
	�� ����ʵ�ʶ������ֽ�����
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
	blkno = openfilelist[fd].first;						// �ļ�����ʼ���
	blkoff = openfilelist[fd].count;					// ����ƫ��
	if(blkoff >= openfilelist[fd].length)
	{
		printf("[Error]do_read: Read out of range!\n");
		free(buf);
		return -1;
	}
	fatptr = fat1 + blkno;
	while (blkoff >= BLOCKSIZE)		// ����ƫ�ƴ��ڿ�Ĵ�С�򽫿����Ϊ��һ��Ŀ�ţ�����ƫ��
	{
		blkno = fatptr->id;
		blkoff = blkoff - BLOCKSIZE;
		fatptr = fat1 + blkno;
	}
	// ��ȡ
	ll = 0;
	while (ll < len)
	{
		blkptr = (unsigned char *)(myvhard + blkno * BLOCKSIZE);
		for (i = 0; i < BLOCKSIZE; i++)					// �����һ���̿�����ݶ�ȡ��buf��
			buf[i] = blkptr[i];
		for (; blkoff < BLOCKSIZE; blkoff++)			// ��buf�У��Ӷ�дָ�봦��ʼ������
		{
			text[ll++] = buf[blkoff];					// �������������ݶ���text��
			openfilelist[fd].count++;					// �ڶ��Ĺ����У���дָ��Ҳ���ű仯
			if (ll == len || openfilelist[fd].count == openfilelist[fd].length)
				break;
		}
		if (ll < len && openfilelist[fd].count != openfilelist[fd].length)	// һ���̿�����ݲ�����
		{
			blkno = fatptr->id;
			if (blkno == END)							// ���ļ�û����һ���̿飬ֹͣ
				break;
			blkoff = 0;									// ����һ���µ��̿��У���дָ��Ҫ��ͷ��ʼ��
			fatptr = fat1 + blkno;						// ʵ�ʵ�ַ
		}
	}
	text[ll] = '\0';
	free(buf);

	return ll;
}

/*
do_write | ʵ��д�ļ�����
����:��д�ļ�����my_write()���ã��������������������д����Ӧ���ļ���ȥ��
����:
	fd:		open()�����ķ���ֵ���ļ�����������
	text:	ָ��Ҫд������ݵ�ָ�룻
	len:	����Ҫ��д���ֽ���
	wstyle:	д��ʽ
���:ʵ��д����ֽ�����
*/
int do_write(int fd, char* text, int len, char wstyle)
{
	/*
	�� ��malloc()����1024B���ڴ�ռ���Ϊ��д���̵Ļ�����buf������ʧ���򷵻�-1������ʾ������Ϣ��
	�� ����дָ��ת��Ϊ�߼����źͿ���ƫ��off��
	  �����ô��ļ�������е��׿�ż�FAT���������ݽ��߼�����ת���ɶ�Ӧ�Ĵ��̿���blkno��
	  ����Ҳ�����Ӧ�Ĵ��̿飬����Ҫ����FATΪ���߼������һ�µĴ��̿飬
	  ������Ӧ�Ĵ��̿���blkno�Ǽǵ�FAT�У�������ʧ�ܣ��򷵻�-1������ʾ������Ϣ��
	�� ����Ǹ���д�����������ǰ��дָ������Ӧ�Ŀ���ƫ��off������0��
	  �򽫿��Ϊblkno��������̿�ȫ��1024B�����ݶ���������buf�У��������ASCII��0���buf��
	�� ��text��δд��������ݴ浽������buff�ĵ�off�ֽڿ�ʼ��λ�ã�ֱ������������
	  ���߽��յ������ַ�CTR+ZΪֹ��������д���ֽ�����¼��tmplen�У�
	�� ��buf��1024B������д�뵽���Ϊblkno��������̿��У�
	�� ����ǰ��дָ���޸�Ϊԭ����ֵ����tmplen����������ʵ��д����ֽ�������tmplen��
	�� ���tmplenС��len����ת�ڼ���д�룻����ת�ࣻ
	�� ���ر���ʵ��д����ֽ�����
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
	while (blkoff >= BLOCKSIZE)		// ����дָ����ڵ����̿��С������ƫ�ƴ��ڿ�Ĵ�С(һ�����̷Ų�����д�ļ�)
	{
		blkno = fatptr1->id;					// ��һ���
		if(blkno == END)						// �ô��̿��ǽ����飬������û����һ��
		{
			blkno = getFreeBLOCK();				// Ѱ�ҿ��п�
			if (blkno == -1)					// û�п��п�
			{
				printf("[Error]do_write: Can't find free blocks.\n");
				free(buf);
				return -1;
			}
			fatptr1->id = blkno;				// д����ʼ���
			fatptr2->id = blkno;
			fatptr1 = fat1 + blkno;
			fatptr2 = fat2 + blkno;
			fatptr1->id = END;
			fatptr2->id = END;
		}
		else									// ������һ��
		{
			fatptr1 = fat1 + blkno;
			fatptr2 = fat2 + blkno;
		}
		blkoff = blkoff - BLOCKSIZE;	// ��������ƫ�ƣ���blkoff��λ���ļ����һ�����̿�Ķ�дλ��
	}
	// д
	ll = 0;
	while (ll < len)
	{
		blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE);		// ָ�����������Ҫд����̿�
		for (i = 0; i < BLOCKSIZE; i++)			// ���̿���ԭ�е����ݿ�����������buf��
			buf[i] = blkptr[i];
		for (; blkoff < BLOCKSIZE; blkoff++)	// ��textָ��ָ������ݿ�����������buf��
		{
			buf[blkoff] = text[ll++];
			openfilelist[fd].count++;
			if (ll == len)
				break;
		}
		for (i = 0; i < BLOCKSIZE; i++)			// �ѻ�����buf�е����ݿ�����Ҫд���������̿���
			blkptr[i] = buf[i];
		if(ll < len)							// ���һ�����̿�д���£����ٷ���һ�����̿�
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
	if (openfilelist[fd].count > openfilelist[fd].length)		// ����дָ�����ԭ���ļ��ĳ��ȣ����޸��ļ��ĳ���
		openfilelist[fd].length = openfilelist[fd].count;
	openfilelist[fd].fcbstate = 1;								// ���޸�
	free(buf);
	return ll;
}

/*
my_read | ���ļ�����
����:����ָ���ļ��дӶ�дָ�뿪ʼ�ĳ���Ϊlen�����ݵ��û��ռ��С�
����:
fd:		open()�����ķ���ֵ���ļ�����������
len:	Ҫ���ļ��ж������ֽ�����
���:ʵ�ʶ������ֽ�����
*/
int my_read(int fd, int len)
{
	/*
	�� ����һ���ַ�������text[len]�����������û����ļ��ж������ļ����ݣ�
	�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1������ʾ������Ϣ��
	�� ����do_read()��ָ���ļ��е�len�ֽ����ݶ�����text[]�У�
	�� ���do_read()�ķ���ֵΪ��������ʾ������Ϣ������text[]�е�������ʾ����Ļ�ϣ�
	�� ���ء�
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
my_mkdir | ������Ŀ¼����
����:�ڵ�ǰĿ¼�´�����Ϊdirname����Ŀ¼��
����:
	dirname:�½�Ŀ¼��Ŀ¼����
*/
void my_mkdir(char* dirname)
{
	/*
	�� ����do_read()���뵱ǰĿ¼�ļ����ݵ��ڴ棬��鵱ǰĿ¼���½�Ŀ¼�ļ��Ƿ��������������򷵻أ�����ʾ������Ϣ��
	�� Ϊ�½���Ŀ¼�ļ�����һ�����д��ļ�������û�п��б����򷵻�-1������ʾ������Ϣ��
	�� ���FAT�Ƿ��п��е��̿飬������Ϊ�½�Ŀ¼�ļ�����һ���̿飬
	  �����ͷŢ��з���Ĵ��ļ�������أ�����ʾ������Ϣ��
	�� �ڵ�ǰĿ¼��Ϊ�½�Ŀ¼�ļ�Ѱ��һ�����е�Ŀ¼���Ϊ��׷��һ���µ�Ŀ¼��;
	  ���޸ĵ�ǰĿ¼�ļ��ĳ�����Ϣ��������ǰĿ¼�ļ����û����ļ������е�fcbstate��Ϊ1��
	�� ׼�����½�Ŀ¼�ļ���FCB�����ݣ��ļ�������ΪĿ¼�ļ���
	  �Ը���д��ʽ����do_write()������д����Ӧ�Ŀ�Ŀ¼���У�
	�� ���½�Ŀ¼�ļ������䵽�Ĵ��̿��н������������Ŀ¼�.���͡�..��Ŀ¼�
	  �����ǣ��������û��ռ���׼�������ݣ�Ȼ���Խض�д���߸���д��ʽ����do_write()����д�����з��䵽�Ĵ��̿��У�
	�� ���ء�
	*/
	fcb *fcbptr;
	fat *fat1, *fat2;
	time_t now;
	struct tm *nowtime;
	char text[MAX_TEXT_SIZE];					// ��Ŵ򿪱����е�����
	unsigned short blkNum;
	int readByteNum, fd, i;
	// ��λ���ļ��������ʼλ��
	fat1 = (fat*)(myvhard + BLOCKSIZE);
	fat2 = (fat*)(myvhard + 3 * BLOCKSIZE);
	openfilelist[curdir].count = 0;				// ��дָ��λ��0
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);

	if(strchr(dirname, '.'))
	{
		printf("[Error]my_mkdir: dirname can't contain '.'!\n");
		return;
	}

	// ����½�Ŀ¼�Ƿ�����
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
	// �ҵ����еĴ򿪱���
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
	// дFCB��Ϣ
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

	fd = my_open(dirname);						// ȡ���½���Ŀ¼���û��򿪱������
	if (fd == -1)
		return;
	// ���½���Ŀ¼�ļ������䵽�Ĵ��̿��н���Ŀ¼��"."��".."
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
my_rmdir | ɾ����Ŀ¼
����:�ڵ�ǰĿ¼��ɾ����Ϊdirname����Ŀ¼��
����:
	dirname:��ɾ��Ŀ¼��Ŀ¼����
	�� ����do_read()���뵱ǰĿ¼�ļ����ݵ��ڴ棬��鵱ǰĿ¼����ɾ��Ŀ¼�ļ��Ƿ���ڣ�
	  ���������򷵻أ�����ʾ������Ϣ��
	�� �����ɾ��Ŀ¼�ļ��Ƿ�Ϊ�գ����ˡ�.���͡�..����û��������Ŀ¼���ļ�����
	  �ɸ�����Ŀ¼���м�¼���ļ��������жϣ�����Ϊ���򷵻أ�����ʾ������Ϣ��
	�� ����Ŀ¼�ļ��Ƿ��Ѿ��򿪣����Ѵ������my_close()�رյ���
	�� ���ո�Ŀ¼�ļ���ռ�ݵĴ��̿飬�޸�FAT��
	�� �ӵ�ǰĿ¼�ļ�����ո�Ŀ¼�ļ���Ŀ¼���free�ֶ���Ϊ0���Ը���д��ʽ����do_write()��ʵ�֣�
	�� �޸ĵ�ǰĿ¼�ļ����û��򿪱����еĳ�����Ϣ�����������е�fcbstate��Ϊ1��
	�� ���ء�
�ܽ�:
1. �ѵ�ǰĿ¼��Ŀ¼�ļ���ȡ��buf��
2. �����������û��ƥ��dirname��Ŀ¼�ļ�
3. ɾ������ļ�Ŀ¼ռ�õ������̿�(fatȫ���ͷţ����ݵ�fat2)
4. ���µ�ǰĿ¼�ļ�����dirname��Ӧ��fcb��������µ�ǰĿ¼�ļ���С
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
	// ����ɾ�� . �� ..
	if(strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
	{
		printf("[Error]my_rmdir: Can't remove this directory.\n");
		return;
	}
	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);
	fcbptr = (fcb*)text;
	// Ѱ��Ҫɾ����Ŀ¼
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
	// ���Ҫɾ����Ŀ¼���ڣ�����Ƿ�Ϊ��
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
	// �޸�Ҫɾ��Ŀ¼��fat����ռ�õ�Ŀ¼������
	while (blkNum != END)
	{
		// ������ռ���̿���Ϊ����
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;
		fatptr1->id = FREE;
		fatptr2->id = FREE;
	}
	my_close(fd);
	strcpy(fcbptr->filename, "");						// �޸���ɾ��Ŀ¼�ڵ�ǰĿ¼��fcb����
	fcbptr->free = 0;
	openfilelist[curdir].count = i * sizeof(fcb);		// ���µ�ǰĿ¼�ļ�������
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
	openfilelist[curdir].fcbstate = 1;
}

/*
my_ls | ��ʾĿ¼����
����: ��ʾ��ǰĿ¼������(��Ŀ¼���ļ���Ϣ)
*/
void my_ls()
{
	/*
	�� ����do_read()������ǰĿ¼�ļ����ݵ��ڴ棻
	�� ��������Ŀ¼�ļ�����Ϣ����һ���ĸ�ʽ��ʾ����Ļ�ϣ�
	�� ���ء�
	*/
	fcb *fcbptr;
	char buf[MAX_TEXT_SIZE];
	int readByteNum, i;

	if(openfilelist[curdir].attribute == 1)			// ����������ļ�
	{
		printf("[Error]my_ls: Can't use ls in data files.\n");
		return;
	}
	openfilelist[curdir].count = 0;
	readByteNum = do_read(curdir, openfilelist[curdir].length, buf);
	fcbptr = (fcb*)buf;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (fcbptr->free)							// ���Ŀ¼���ѷ���
		{
			if (fcbptr->attribute == 0)				// Ŀ¼
				printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n",
					fcbptr->filename,
					(fcbptr->date >> 9) + 1980,
					(fcbptr->date >> 5) & 0x000f,
					fcbptr->date & 0x001f,
					fcbptr->time >> 11,
					(fcbptr->time >> 5) & 0x003f,
					fcbptr->time & 0x001f * 2);
			else									// �����ļ�
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
my_create | �����ļ�����
����:������Ϊfilename�����ļ���
����:
	filename:�½��ļ����ļ��������ܰ���·����
���:�������ɹ������ظ��ļ����ļ����������ļ��򿪱��е������±꣩�����򷵻�-1��
�� Ϊ���ļ�����һ�����д��ļ�������û�п��б����򷵻�-1������ʾ������Ϣ��
�� �����ļ��ĸ�Ŀ¼�ļ���û�д򿪣������my_open()�򿪣�
  ����ʧ�ܣ����ͷŢ���Ϊ�½��ļ�����Ŀ����ļ��򿪱������-1������ʾ������Ϣ��
�� ����do_read()�����ø�Ŀ¼�ļ����ݵ��ڴ棬����Ŀ¼�����ļ��Ƿ����������������ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ���Ȼ�󷵻�-1������ʾ������Ϣ��
�� ���FAT�Ƿ��п��е��̿飬������Ϊ���ļ�����һ���̿飬�����ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ�������-1������ʾ������Ϣ��
�� �ڸ�Ŀ¼��Ϊ���ļ�Ѱ��һ�����е�Ŀ¼���Ϊ��׷��һ���µ�Ŀ¼��;���޸ĸ�Ŀ¼�ļ��ĳ�����Ϣ��������Ŀ¼�ļ����û����ļ������е�fcbstate��Ϊ1��
�� ׼�������ļ���FCB�����ݣ��ļ�������Ϊ�����ļ�������Ϊ0���Ը���д��ʽ����do_write()������д�����з��䵽�Ŀ�Ŀ¼���У�
�� Ϊ���ļ���д���з��䵽�Ŀ��д��ļ����fcbstate�ֶ�ֵΪ0����дָ��ֵΪ0��
�� ����my_close()�رբ��д򿪵ĸ�Ŀ¼�ļ���
�� �����ļ��Ĵ��ļ����������Ϊ���ļ����������ء�
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
	// ��ȡ��ǰĿ¼�µ��ļ���Ϣ������ļ��Ƿ��Ѿ�����
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
	// ��ȡ����fcb���̿�
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
	// ���Ը�ֵ
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
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);			// ���½��ļ���Ŀ¼��д�뵽��ǰĿ¼�ļ���

	fcbptr = (fcb *)text;
	fcbptr->length = openfilelist[curdir].length;
	openfilelist[curdir].count = 0;
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);			// �޸ĵ�ǰĿ¼�ĳ���
	openfilelist[curdir].fcbstate = 1;
}

/*
my_rm | ɾ���ļ�����
����:ɾ����Ϊfilename���ļ���
����:
	filename:��ɾ���ļ����ļ��������ܻ�����·����
	�� ����ɾ���ļ��ĸ�Ŀ¼�ļ���û�д򿪣������my_open()�򿪣�����ʧ�ܣ��򷵻أ�����ʾ������Ϣ��
	�� ����do_read()�����ø�Ŀ¼�ļ����ݵ��ڴ棬����Ŀ¼����ɾ���ļ��Ƿ���ڣ����������򷵻أ�����ʾ������Ϣ��
	�� �����ļ��Ƿ��Ѿ��򿪣����Ѵ���رյ���
	�� ���ո��ļ���ռ�ݵĴ��̿飬�޸�FAT��
	�� ���ļ��ĸ�Ŀ¼�ļ�����ո��ļ���Ŀ¼���free�ֶ���Ϊ0���Ը���д��ʽ����do_write()��ʵ�֣���
	�� �޸ĸø�Ŀ¼�ļ����û����ļ������еĳ�����Ϣ�������ñ����е�fcbstate��Ϊ1��
	�� ���ء�
�ܽ�:
1. ��ȡ��ǰĿ¼��Ŀ¼�ļ���buf
2. ��buf��Ѱ��ƥ��filename�������ļ�
3. �������ļ�ռ�ݵ��̿�(�ͷ�fat�����ұ���)
4. �ڸ�Ŀ¼�ļ��У�ɾ��filename��Ӧ��fcb
5. ���¸�Ŀ¼�ļ��ĳ���
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
	readByteNum = do_read(curdir, openfilelist[curdir].length, text);		// ��ȡ��ǰĿ¼
	// Ѱ��ƥ����ļ�
	fcbptr = (fcb*)text;
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
			break;
		fcbptr++;
	}
	// ���������
	if(i == readByteNum / sizeof(fcb))
	{
		printf("[Error]my_rm: This file does not exist.\n");
		return;
	}
	// �������
	blkNum = fcbptr->first;
	while (blkNum != END)									// ������ļ�ռ�õ����д��̿�
	{
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;
		fatptr1->id = FREE;
		fatptr2->id = FREE;
	}
	// FCB����
	strcpy(fcbptr->filename, "");
	fcbptr->free = 0;
	openfilelist[curdir].count = i * sizeof(fcb);
	do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
	openfilelist[curdir].fcbstate = 1;
}

/*
my_open | ���ļ�����
����:�򿪵�ǰĿ¼����Ϊfilename���ļ���
����:
	filename:�����ļ����ļ���
���:���򿪳ɹ������ظ��ļ��������������û����ļ����б�����ţ������򷵻�-1��
	�� �����ļ��Ƿ��Ѿ��򿪣����Ѵ��򷵻�-1������ʾ������Ϣ��
	�� ����do_read()������Ŀ¼�ļ������ݵ��ڴ棬����Ŀ¼�������ļ��Ƿ���ڣ�
	  ���������򷵻�-1������ʾ������Ϣ��
	�� ����û����ļ������Ƿ��пձ��������Ϊ�����ļ�����һ���ձ��
	  ��û���򷵻�-1������ʾ������Ϣ��
	�� Ϊ���ļ���д�հ��û����ļ���������ݣ���дָ����Ϊ0��
	�� �����ļ������䵽�Ŀհ��û����ļ��������ţ������±꣩��Ϊ�ļ�������fd���ء�
�ܽ�:
�ѵ�ǰĿ¼�ļ���ȡ��buf��,buf�����һ����fcb������Щfcb��Ѱ��filenameƥ����ļ�
Ȼ���openfilelist��ȡһ�������������ļ�д��ȥ���л�currfd=fd��������ļ�
*/
int my_open(char* filename)
{
	fcb *fcbptr;
	char *fname, exname[EXNAME_SIZE], *str, text[MAX_TEXT_SIZE];
	int readByteNum, fd, i;

	fname = strtok(filename, ".");					// �ֳ��ļ���
	str = strtok(NULL, ".");						// �ֳ���չ��
	
	if (str)										// �������չ��
		strcpy(exname, str);
	else											// ���û����չ��
		strcpy(exname, "");
	// ���û����ļ�������ҵ�ǰ�ļ��Ƿ��Ѿ���
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
	// Ѱ���ļ�,�ҵ���λfcbptr
	for (i = 0; i < readByteNum / sizeof(fcb); i++)
	{
		if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
			break;
		fcbptr++;
	}
	// û���ҵ��ļ�
	if (i == readByteNum / sizeof(fcb))
	{
		printf("[Error]my_open: This file doesn't exist.\n");
		return -1;
	}
	fd = getFreeOpenfilelist();						// Ѱ�ҿ����ļ�����
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
	//�ļ�Ŀ¼��; 
	strcpy(openfilelist[fd].dir, openfilelist[curdir].dir);
	strcat(openfilelist[fd].dir, filename);
	if (fcbptr->attribute & 0x20)
		strcat(openfilelist[fd].dir, "\\");
	openfilelist[fd].father = curdir;
	openfilelist[fd].count = 0;
	openfilelist[fd].fcbstate = 0;					//δ�޸�FCB; 
	openfilelist[fd].topenfile = 1;					//�򿪱���״̬��Ϊ��ռ��; 
	return fd;
}

/*
my_close | �ر��ļ�����
����:�ر�ǰ����my_open()�򿪵��ļ�������Ϊfd���ļ���
����:
	fd:�ļ���������

	�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1��
	�� ����û����ļ�������е�fcbstate�ֶε�ֵ�����Ϊ1����Ҫ�����ļ���FCB�����ݱ��浽��������ϸ��ļ���Ŀ¼���У������ǣ��򿪸��ļ��ĸ�Ŀ¼�ļ����Ը���д��ʽ����do_write()�����ر��ļ���FCBд�븸Ŀ¼�ļ�����Ӧ�̿��У�
	�� ���ո��ļ�ռ�ݵ��û����ļ�����������ղ�����������topenfile�ֶ���Ϊ0��
	�� ���ء�
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
	����ļ�fcb���޸Ĺ���Ҫд��ȥ
	 *�ȰѸ�Ŀ¼�ļ��Ӵ����ж�ȡ��buf�У�Ȼ��Ѹ��º��fcb����д��buf�Ȼ���ٴ�bufд������
	 *д�ش���ʱ��ֻҪ��countָ������ļ���Ӧ��fcbλ�ã�Ȼ��д��fcb����
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
		//diroffΪ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ���dirno�̿��е�Ŀ¼�����; 
		openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);
		//�Ը���д��ʽ�����رյ��ļ���FCBд�븸Ŀ¼�ļ�����Ӧ�̿���; 
		do_write(father, (char *)fcbptr, sizeof(fcb), 2);
		//����; 
		free(fcbptr);
		openfilelist[fd].fcbstate = 0;
	}
	//���մ򿪱���; 
	strcpy(openfilelist[fd].filename, "");
	strcpy(openfilelist[fd].exname, "");
	openfilelist[fd].topenfile = 0;
	father = openfilelist[fd].father;
	return father;//���ظ�·�����ļ�������(�򿪱������); 
}

/*
my_write | д�ļ�����
���ܣ����û�ͨ���������������д��fd��ָ�����ļ��С�
����:
	fd:		open()�����ķ���ֵ���ļ�����������
�����ʵ��д����ֽ�����

1.�ض�д:	����ԭ���ļ������ݣ�����д�ļ�
2.����д:	�޸��ļ��ڵ�ǰ��дָ����ָ��λ�ÿ�ʼ�Ĳ������ݣ�
3.׷��д:	����ԭ�ļ����������µ����ݡ�
�ض�д:�ͷ��ļ����ڵĳ���һ����������̿�(��Ҫʱ�ٷ���)������дָ����0; 
����д:ֱ��; 
׷��д:��дָ��ŵ��ļ�ĩβ;
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
		//1.�ض�д 2.����д 3.׷��д
		printf("Enter write style:\n1. Cut write\t2. Cover write\t3. Add write\n");
		scanf("%d", &wstyle);
		if (wstyle > 0 && wstyle < 4)
			break;
		printf("[Error]my_write: Write style error!\n");
	}
	getchar();
	switch (wstyle)
	{
	case 1:								// �ض�д����Դ�ļ���ռ��������̿ռ�����Ϊ1
		blkNum = openfilelist[fd].first;
		fatptr1 = fat1 + blkNum;
		fatptr2 = fat2 + blkNum;
		blkNum = fatptr1->id;			// ��ȡfat���еڶ�����
		fatptr1->id = END;				// ��һ�����
		fatptr2->id = END;
		// �������һ��
		while (blkNum != END)
		{
			fatptr1 = fat1 + blkNum;
			fatptr2 = fat2 + blkNum;
			blkNum = fatptr1->id;
			fatptr1->id = FREE;
			fatptr2->id = FREE;
		}
		// ��дָ����ļ�������0
		openfilelist[fd].count = 0;
		openfilelist[fd].length = 0;
		break;
	case 2:								// ����д
		openfilelist[fd].count = 0;
		break;
	case 3:								// ׷��д
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
	printf("*********************Scarb�ļ�ϵͳ V0.0*******************************\n\n");
	printf("������\t\t�������\t\t����˵��\n\n");
	printf("cd\t\tĿ¼��(·����)\t\t�л���ǰĿ¼��ָ��Ŀ¼\n");
	printf("mkdir\t\tĿ¼��\t\t\t�ڵ�ǰĿ¼������Ŀ¼\n");
	printf("rmdir\t\tĿ¼��\t\t\t�ڵ�ǰĿ¼ɾ��ָ��Ŀ¼\n");
	printf("ls\t\t��\t\t\t��ʾ��ǰĿ¼�µ�Ŀ¼���ļ�\n");
	printf("create\t\t�ļ���\t\t\t�ڵ�ǰĿ¼�´���ָ���ļ�\n");
	printf("rm\t\t�ļ���\t\t\t�ڵ�ǰĿ¼��ɾ��ָ���ļ�\n");
	printf("open\t\t�ļ���\t\t\t�ڵ�ǰĿ¼�´�ָ���ļ�\n");
	printf("write\t\t��\t\t\t�ڴ��ļ�״̬�£�д���ļ�\n");
	printf("read\t\t��\t\t\t�ڴ��ļ�״̬�£���ȡ���ļ�\n");
	printf("close\t\t��\t\t\t�ڴ��ļ�״̬�£��رո��ļ�\n");
	printf("exit\t\t��\t\t\t�˳�ϵͳ\n\n");
	printf("*********************************************************************\n\n");
	while(flag)
	{
		printf("%s>", openfilelist[curdir].dir);
		gets_s(s);		// gets()
		cmdn = -1;
		if(strcmp(s, ""))
		{
			sp = strtok(s, " ");				// ���ݿո�ָ��ַ���
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
					printf("��������ȷ��ָ��.\n");
				break;
			case 1:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_mkdir(sp);
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 2:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_rmdir(sp);
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 3:
				if (openfilelist[curdir].attribute == 0)
					my_ls();
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 4:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_create(sp);
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 5:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
					my_rm(sp);
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 6:
				sp = strtok(NULL, " ");
				if (sp && (openfilelist[curdir].attribute == 0))
				{
					if (strchr(sp, '.'))//����sp��'.'�״γ��ֵ�λ��
						curdir = my_open(sp);
					else
						printf("�򿪵��ļ�û�к�׺��.\n");
				}
				else
					printf("��������ȷ��ָ��.\n");
				break;
			case 7:
				if (!(openfilelist[curdir].attribute == 0))
					curdir = my_close(curdir);
				else
					printf("�ļ���ʧ��.\n");
				break;
			case 8:
				if (!(openfilelist[curdir].attribute == 0))
					my_write(curdir);
				else
					printf("�ļ���ʧ��.\n");
				break;
			case 9:
				if (!(openfilelist[curdir].attribute == 0))
					my_read(curdir, openfilelist[curdir].length);
				else
					printf("�ļ���ʧ��.\n");
				break;
			case 10:
				if (openfilelist[curdir].attribute == 0)
				{
					my_exitsys();
					flag = 0;
				}
				else
					printf("��������ȷ��ָ��.\n");
				break;
			default:
				printf("��������ȷ��ָ��.\n");
				break;
			}
		}
	}
}