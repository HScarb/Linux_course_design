#include<linux/list.h>
#include<linux/init.h>
#include<stdio.h>

int main()
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
		p = list_enrty(pos, struct task_struct, tasks);
		count++;
		printf("%d\t%s\n", p->pid, p->comm);
	}
	printf("Total processes: %d\n", count);
	return 0;
}

