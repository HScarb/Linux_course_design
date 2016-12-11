#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/list.h>
#include<linux/sched.h>
#include<linux/export.h>

MODULE_LICENSE("GPL");
MODULE_LICENSE("Scarb");
MODULE_DESCRIPTION("List all process in kernal");

static int __init hello_init(void)
{
        struct task_struct *p;
//        struct list_head *pos;
        int count;
//        task = NULL;
//        p = NULL;
//        pos = NULL;
        count = 0;
//        task = &init_task;

	printk("Module list_all_process load.\n");
	printk("pid\tname\t\tprio\tstate\n");
        for_each_process(p)
        {
                // p = list_entry(pos, struct task_struct, tasks);
                if(p->mm == NULL)
		{
			count++;
                	printk("%d\t%s\t\t%d\t%ld\n", p->pid, p->comm, p->prio, p->state);
		}
        }
        printk("Total processes: %d\n", count);
        return 0;  
}         
         
static void __exit hello_exit(void)
{
        printk("Module list_all_process exit.\n");
}     
module_init(hello_init);
module_exit(hello_exit);

