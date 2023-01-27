/*
Operating System Project 2: get_trace.c

The implementation of system call get_trace.
It return the page writes frequency wcounts of the given process pid.
The system call number is 363.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>

MODULE_LICENSE("Dual BSD/GPL");
#define __NR_sys_get_trace 363

static int (*oldcall)(void);


void* sys_get_trace(pid_t pid, int *wcounts)
{
    struct task_struct *tsk;

    /* get the task_struct according to pid */
    tsk=pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!tsk){
        return -1;
    }

    /* return the wcounts */
    *wcounts=tsk->wcounts;
    printk("get_trace:: pid: %d\n", pid);

    return 0;
}

static int addsyscall_init(void)
{
    long *syscall = (long *)0xc000d8c4;
    oldcall = (int (*)(void))(syscall[__NR_sys_get_trace]);
    syscall[__NR_sys_get_trace] = (unsigned long)sys_get_trace;
    printk(KERN_INFO "module load!\n");
    return 0;
}

static void addsyscall_exit(void)
{
    long *syscall = (long *)0xc000d8c4;
    syscall[__NR_sys_get_trace] = (unsigned long)oldcall;
    printk(KERN_INFO "module exit!\n");
}
module_init(addsyscall_init);
module_exit(addsyscall_exit);
