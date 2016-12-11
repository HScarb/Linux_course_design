#include<linux/init.h>
#include<linux/list.h>		// for list_for_each
#include<linux/sched.h>		// for init_task
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/moduleparam.h>
MODULE_LICENSE("GPL");
//static char *who;		// param declartion
//static int times;
//module_param(who, charp, 0644);		// param info
//module_param(times, int, 0644);
static int __init hello_init(void)
{
	struct task_struct *task, *p; 
        struct list_head *pos;
        int count;
        task = NULL;
        p = NULL;
        pos = NULL;
        count = 0;
        task = &init_task;

        list_for_each(pos, &task->tasks)
        {
                p = list_entry(pos, struct task_struct, tasks);
                count++;
                printk("%d\t%s\n", p->pid, p->comm);
        }
        printk("Total processes: %d\n", count);
        return 0; 
	
}

static void __exit hello_exit(void)
{
	printk("Goodbye");
}
module_init(hello_init);
module_exit(hello_exit);
