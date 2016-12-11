#include<linux/init.h>
#include<linux/list.h>          // for list_for_each
#include<linux/sched.h>         // for init_task
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/moduleparam.h>
MODULE_LICENSE("GPL");

static int pid;
module_param(pid, int, 0644);		// param pid

static int __init hello_init(void)
{
        struct task_struct *task, *p;
        struct list_head *pos;
        int count;
        task = NULL;
        p = NULL;
        pos = NULL;
        count = 0;
	//pid_t vpid = (pid_t)pid;
	struct pid * spid = find_get_pid(pid);
        task = get_pid_task(spid, 0);
	printk("PID\tName\n");
	printk("Task find: \n%d\t%s\n", task->pid, task->comm);
	// parent
	printk("Parent:\n");	
//	print("%d\t%s\n");
	printk("%d\t%s\n", task->parent->pid, task->parent->comm);
	// children
	pos = NULL;
	printk("Children:\n");	
        list_for_each(pos, &task->children)	// get children head, go through siblings
        {
                p = list_entry(pos, struct task_struct, sibling);
                //count++;
                printk("ID:%d\tN:%s\n", p->pid, p->comm);
        }

	// sibling
	pos = NULL;
	printk("Sibling:\n");
	list_for_each(pos, &task->parent->children)	// get head, go through siblings
	{
		p = list_entry(pos, struct task_struct, sibling);
		printk("ID:%d\tN:%s\n", p->pid, p->comm);
	}
//        printk("Total processes: %d\n", count);
        return 0; 
         
}         
         
static void __exit hello_exit(void)
{
        printk("Goodbye");
}     
module_init(hello_init);
module_exit(hello_exit);

