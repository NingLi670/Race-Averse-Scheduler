/*
Operating System Project 2: set_sched.c

Change the scheduler of specified task.
Test if we can change a task's scheduler into RAS.
*/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3
#define SCHED_IDLE 5
#define SCHED_RAS 6

char *SCHED_NAME[] = {"SCHED_NORMAL", "SCHED_FIFO", "SCHED_RR",
					  "SCHED_BATCH", "", "SCHED_IDLE", "SCHED_RAS"};

/* retuen scheduler and handle exception */
int get_policy(pid_t pid)
{
	int policy;
	policy = sched_getscheduler(pid);
	if (policy >= 0 && policy <= 6 && policy != 4)
	{
		return policy;
	}
	else if (policy == -1)
	{
		switch (errno)
		{
		case ENOSYS:
			printf("The function sched_getscheduler() is not supported by this implementation. \n");
			break;
		case EPERM:
			printf("The requesting process does not have permission to determine the scheduling policy of the specified process. \n");
			break;
		case ESRCH:
			printf("No process can be found corresponding to that specified by pid. \n");
			break;
		default:
			break;
		}
		return 4;
	}
	else
	{
		printf("Union scheduling policy.\n");
		return 4;
	}
}

/* set scheduler and handle exception */
void set_policy(pid_t pid, int policy, struct sched_param param)
{
	if (sched_setscheduler(pid, policy, &param))
	{
		switch (errno)
		{
		case EINVAL:
			printf("The value of the policy parameter is invalid, or one or more of the parameters contained in param is outside the valid range for the specified scheduling policy. \n");
			break;
		case ENOSYS:
			printf("The function sched_setscheduler() is not supported by this implementation. \n");
			break;
		case EPERM:
			printf("The requesting process does not have permission to set either or both of the scheduling parameters or the scheduling policy of the specified process. \n");
			break;
		case ESRCH:
			printf("No process can be found corresponding to that specified by pid. \n");
			break;
		default:
			break;
		}
	}
}

int main()
{
	pid_t pid;
	int policy;
	struct sched_param param;

	/* input scheduling policy */
	printf("Please input the choice of scheduling algorithms ");
	printf("(0-NORMAL, 1-FIFO, 2-RR, 6-RAS) : ");
	scanf("%d", &policy);
	while (policy != 0 && policy != 1 && policy != 2 && policy != 6)
	{
		printf("Please input the valid scheduling policy number: ");
		scanf("%d", &policy);
	}
	printf("Current scheduling algorithm is %s\n", SCHED_NAME[policy]);

	/* input pid and start trace */
	printf("Please input the id(PID) of the testprocess : ");
	scanf("%d", &pid);
	syscall(361, pid);
	printf("Start trace for task : %d\n", pid);

	/* set priority for SCHED_FIFO and SCHED_RR */
	if (policy == 1 || policy == 2)
	{
		/* only SCHED_FIFO and SCHED_RR needs to input priority */
		printf("Set process's priority (1-99): ");
		scanf("%d", &param.sched_priority);
		printf("current scheduler's priority is : %d\n", param.sched_priority);
	}
	else
	{
		param.sched_priority=0;
		printf("current scheduler's priority is : 0\n");
	}

	/* switch scheduler, output previous and current scheduler */
	printf("pre scheduler : %s\n", SCHED_NAME[get_policy(pid)]);
	set_policy(pid, policy, param);
	printf("cur scheduler : %s\n", SCHED_NAME[get_policy(pid)]);
	printf("Switch finish.\n");
}
