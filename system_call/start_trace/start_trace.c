/*
Operating System Project 2: start_trace.c

The implementation of system call strat_trace.
It tells the kernel to start tracing page writes for the given process pid.
The system call number is 361.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>

MODULE_LICENSE("Dual BSD/GPL");
#define __NR_sys_start_trace 361

static int (*oldcall)(void);

void *sys_start_trace(pid_t pid, unsigned long start, size_t size)
{
    struct task_struct *tsk;

    /* get the task_struct according to pid */
    tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!tsk)
    {
        return -1;
    }

    /* return -EINVAL if called twice without sys_stop_trace being called in between */
    if (tsk->trace_flag)
    {
        return -EINVAL;
    }

    /* set the trace_flag and initialize wcounts */
    tsk->trace_flag = true;
    tsk->wcounts = 0;
    printk("start_trace:: pid: %d\n", pid);

    return 0;
}

static int addsyscall_init(void)
{
    long *syscall = (long *)0xc000d8c4;
    oldcall = (int (*)(void))(syscall[__NR_sys_start_trace]);
    syscall[__NR_sys_start_trace] = (unsigned long)sys_start_trace;
    printk(KERN_INFO "module load!\n");
    return 0;
}

static void addsyscall_exit(void)
{
    long *syscall = (long *)0xc000d8c4;
    syscall[__NR_sys_start_trace] = (unsigned long)oldcall;
    printk(KERN_INFO "module exit!\n");
}
module_init(addsyscall_init);
module_exit(addsyscall_exit);