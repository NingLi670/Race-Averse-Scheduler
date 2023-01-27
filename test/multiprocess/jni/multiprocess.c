/*
Operating System Project 2: multiprocess.c

Fork several child tasks to test the RAS scheduler.
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
#include <stdlib.h>

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3
#define SCHED_IDLE 5
#define SCHED_RAS 6

char *SCHED_NAME[] = {"SCHED_NORMAL", "SCHED_FIFO", "SCHED_RR",
					  "SCHED_BATCH", "", "SCHED_IDLE", "SCHED_RAS"};

static int alloc_size;
static char *memory;

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
static void set_policy(pid_t pid, int policy, struct sched_param *param)
{
	if (sched_setscheduler(pid, policy, param))
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

void segv_handler(int signal_number)
{
	mprotect(memory, alloc_size, PROT_READ | PROT_WRITE);
}

/* randomly write data to memory */
void memory_write(int n)
{
	int fd, i;
	struct sigaction sa;

	/* Init segv_handler to handle SIGSEGV */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &segv_handler;
	sigaction(SIGSEGV, &sa, NULL);

	/* allocate memory for process, set the memory can only be read */
	alloc_size = 10 * getpagesize();
	fd = open("/dev/zero", O_RDONLY);
	memory = mmap(NULL, alloc_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	/* try to write, will receive a SIGSEGV */
	memory[0] = 0;

	for (i = 1; i < n; i++)
	{
		/* set protection */
		mprotect(memory, alloc_size, PROT_READ);
		/* try to write, will receive a SIGSEGV */
		memory[rand() % alloc_size] = i;
	}

	/* freee */
	munmap(memory, alloc_size);
}

int main()
{
	pid_t pid;
	int n, i;
	struct sched_param param;
	int randnum[100];

	param.sched_priority = 0;
	srand(time(0));

	printf("Please input the number of process: ");
	scanf("%d", &n);

	printf("Parent process pid: %d\n", getpid());

	for (i = 0; i < n; i++)
	{
		randnum[i] = rand() % 90 + 10;
	}

	/* fork several child tasks */
	for (i = 0; i < n; i++)
	{
		if ((pid = fork()) == 0)
		{
			sleep(1);
			pid = getpid();
			syscall(361, pid); // start trace

			/* change wcounts by randomly writing to memory */
			memory_write(randnum[i]);

			/* change scheduler to RAS */
			printf("pid: %d, pre scheduler: %s\n", pid, SCHED_NAME[get_policy(pid)]);
			set_policy(pid, SCHED_RAS, &param);
			printf("pid: %d, cur scheduler: %s\n", pid, SCHED_NAME[get_policy(pid)]);

			/* do something to consume timeslice and
			   change wcounts by randomly writing to memory */
			memory_write(1000);
			syscall(362, pid); // stop trace

			exit(0);
		}
		else
		{
			printf("create child %d ,pid: %d\n", i + 1, pid);
		}
	}

	for (i = 0; i < n; i++)
	{
		wait(0);
	}

	return 0;
}
