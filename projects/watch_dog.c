#define _XOPEN_SOURCE 700		/* sigaction struct */

#include <stddef.h>		/* size_t */
#include <stdio.h>		/* printf */
#include <sys/types.h>	/* pid_t */
#include <unistd.h>		/* waitpid, fork, write */
#include <sys/wait.h>	/* waitpid */
#include <signal.h>		/* sigaction, sigset_t */
#include <fcntl.h>		/* O_* constants (O_CREAT)*/
#include <semaphore.h>	/* POSIX semaphore functions and definitions */
#include <pthread.h>	/* POSIX pthreads */
#include <stdatomic.h>	/* atomic types */
#include <string.h>		/* snprintf */
#include <stdlib.h>		/* getenv, atoi */

#include "watch_dog.h"
#include "scheduler.h"
#include "uid.h"

#define WATCH_DOG_PATH "/home/itay/git/projects/watchdog_exec.out"
#define SEMAPHORE_NAME "/wd_sem"
#define SEM_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) /* 0644 in octal */
	/*S_IRUSR: Read permission for the owner*/
	/*S_IWUSR: Write permission for the owner*/
	/*S_IRGRP: Read permission for the group*/
	/*S_IROTH: Read permission for others*/
#define ENVIRONMENT_VAR "WD_RUNNING"
#define UNUSED(var) (void)(var)

/* forward function declerations (grouped by category)*/
static int InitScheduler(void);

static int InitSIGUSR1Handler(void);
static int BlockSIGUSR1(void);
static int UnblockSIGUSR1(void);

static void SignalHandleReceivedLifeSign(int signum);
static void SignalHandleReceivedDNR(int signum);

static int SchedulerActionSendSignal(void *param);
static int SchedulerActionIncreaseCounter(void *param);

static int ReviveWD(void);
static int CreateWDProcess(void);

static int CreateCommunicationThread(void);
static void *ThreadCommunicateWithWD(void *param);



/* global static variables */
static volatile atomic_int repetition_counter = ATOMIC_VAR_INIT(0);
static pid_t g_wd_pid = 0;
static sem_t thread_ready_sem;
static sem_t *process_sem;
static scheduler_t *scheduler;
static char *user_exec_path;
static size_t max_repetitions;
static size_t interval;
struct sigaction old_sig_action;
static pthread_t communication_thread;


/** thread function **/
static void *ThreadCommunicateWithWD(void *param)
{
	printf("client process, thread func start\n");
	
	InitSIGUSR1Handler();	/* error check? */
	
	/* init scheduler and load tasks */
	InitScheduler();	/* how do i know it failed? */
	
	/* update main thread that communication is ready */
	sem_post(&thread_ready_sem);
	printf("client process, thread func, after posting to main thread, starting scheduler\n");
	
	/* run scheduler */
	SchedulerRun(scheduler);
	
	/* will only reach this part after receiving DNR */
	SchedulerDestroy(scheduler);
	UNUSED(param);
	return NULL;
}



int MMI(size_t interval_in_seconds, size_t repetitions, char **argv)
{
	int return_status = 0;
	char *env_wd_running = NULL;
	user_exec_path = argv[0];
	max_repetitions = repetitions;
	interval = interval_in_seconds;
	
	/* semaphore to sync between current process and WD process */
	process_sem = sem_open(SEMAPHORE_NAME, O_CREAT, SEM_PERMISSIONS, 0);
    
	sem_init(&thread_ready_sem, 0, 0);
	
	printf("client process, MMI func, before fork+exec\n");
	
	
	/* if envirnent variable exists - WD process is already running, don't create new one */
	env_wd_running = getenv(ENVIRONMENT_VAR);
	if (env_wd_running != NULL)
	{
		printf("****WD process alive. env var is %s\n", env_wd_running);
		g_wd_pid = atoi(env_wd_running);
		
		/* notify WD process that client process is ready */
		sem_post(process_sem);
	}
	
	/* else - WD process does not exist yet, create WD process */
	else
	{
		printf("****environ var not found, creating WD process\n");
		/* fork + exec WD process */
		return_status = CreateWDProcess();
		if (return_status != 0)
		{
			return return_status;
		}
		
		/* wait for the WD process to initialize */
		sem_wait(process_sem);
		
		printf("client process, MMI func, after fork+exec (after WD proc initialized), before thread_create\n");
	}
	
	/* mask SIGUSR1 on main thread. it will be un-masked in the thread function */
    BlockSIGUSR1();

	/* create communication thread */
	return_status = CreateCommunicationThread();
	if (return_status != 0)
	{
		return return_status;
	}
	
	
	return return_status;
}


void DNR(void)
{
	/* signal WD process to stop and die */
	kill(g_wd_pid, SIGUSR2);
	
	/* stop and cleanup scheduler (cleanup happens in communication thread) */
	SchedulerStop(scheduler);
	
	/* cleanup semaphores */
	sem_destroy(&thread_ready_sem);
	sem_close(process_sem);
	sem_unlink(SEMAPHORE_NAME);
	
	/* cleanup WD environment var */
	unsetenv(ENVIRONMENT_VAR);
	
	/* reset global repetition counter */
	atomic_store(&repetition_counter, 0);
	
	/* un-mask SIGUSR1 in main thread */
	UnblockSIGUSR1();
	
	/* wait for communication thread to cleanup */
	pthread_join(communication_thread, NULL);
	
	/* wait for WD process to cleanup */
	waitpid(g_wd_pid, NULL, 0);
}


static int CreateWDProcess(void)
{
	int return_status = 0;
	char interval_str[16];
    char max_repetitions_str[16];
    
	/* convert integers to strings for exec argv argument */
    snprintf(interval_str, sizeof(interval_str), "%lu", interval);
    snprintf(max_repetitions_str, sizeof(max_repetitions_str), "%lu", max_repetitions);
    
	g_wd_pid = fork();
	
	if (g_wd_pid == 0)	/* in child (WD) process */
	{
		return_status = execl(WATCH_DOG_PATH, WATCH_DOG_PATH, user_exec_path, interval_str, max_repetitions_str, NULL);
		printf("Exec failed. return status = %d\n", return_status);
	}
	else				/* in parent process */
	{
		return return_status;
	}
	
	return return_status;
}


static int CreateCommunicationThread(void)
{
/*	pthread_t communication_thread;*/
	pthread_attr_t attr;
	int return_status = 0;
	
	pthread_attr_init(&attr);
/*	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);*/
	
	return_status = pthread_create(&communication_thread, &attr, &ThreadCommunicateWithWD, NULL);
	
	if (return_status != 0)
	{
	    fprintf(stderr, "failed to create thread\n");
	    return return_status;
	}
	
	/* wait for thread to set up, then return and exit function */
	sem_wait(&thread_ready_sem);
	pthread_attr_destroy(&attr);
	
	return return_status;
}



/** signal handlers **/



static int InitSIGUSR1Handler(void)
{
    struct sigaction sa1 = {0};
	sigset_t set;
	
	/* unblock SIGUSR1 in current calling thread */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	/* set signal handler for SIGUSR1 and save old handler */
	sa1.sa_handler = SignalHandleReceivedLifeSign;
	sa1.sa_flags = 0;
	sigemptyset(&sa1.sa_mask);
	if (sigaction(SIGUSR1, &sa1, &old_sig_action) == -1) 
	{
		perror("sigaction\n");
		return 1;
	}
	
	return 0;
}


static int BlockSIGUSR1(void)
{
	sigset_t set;

	/* mask SIGUSR1 in the main thread */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	return 0;
}


static int UnblockSIGUSR1(void)
{
	if (sigaction(SIGUSR1, &old_sig_action, NULL) == -1)
	{
		perror("sigaction\n");
		return 1;
	}
	
	return 0;
}



static void SignalHandleReceivedLifeSign(int signum)
{
	printf("Client process\t Signal Handler\t received life sign from WD. zeroing counter\n");
	atomic_store(&repetition_counter, 0);
	
	UNUSED(signum);
}


static void SignalHandleReceivedDNR(int signum)
{
	atomic_store(&repetition_counter, 0);
	SchedulerStop(scheduler);
	
	UNUSED(signum);
}



static int InitScheduler(void)
{
	scheduler = SchedulerCreate();
	if (scheduler == NULL)
	{
		return 1;	
	}
	
	/* check for fail */
	SchedulerAddTask(scheduler, SchedulerActionSendSignal, NULL, NULL, interval);
	SchedulerAddTask(scheduler, SchedulerActionIncreaseCounter, NULL, NULL, interval);
	
	return 0;
}


/** scheduler action functions **/
static int ReviveWD(void)
{
	/* reset counter and scheduler and wait for new WD process */
	SchedulerStop(scheduler);
	atomic_store(&repetition_counter, 0);
	
	CreateWDProcess();	/* check for fail? */
	
	/* wait for the WD process to initialize */
	sem_wait(process_sem);
	printf("\tClient process\t received post after WD re-created. resuming scheduler\n");
	/* resume scheduler */
	SchedulerAddTask(scheduler, SchedulerActionIncreaseCounter, NULL, NULL, interval);
	SchedulerRun(scheduler);
	
	return 0;
}


static int SchedulerActionSendSignal(void *param)
{
	printf("Client process\t Action Function\tsending SIGUSR1 to (WD = %d) process\n", g_wd_pid);
	kill(g_wd_pid, SIGUSR1);
	
	UNUSED(param);
	return 0;
}


static int SchedulerActionIncreaseCounter(void *param)
{
	int current_count = 0;
	
	atomic_fetch_add(&repetition_counter, 1);
	current_count = atomic_load(&repetition_counter);
	printf("Client process\t Action Function\tincrease counter func. current count = %d\n", current_count);
    if ((size_t)current_count == max_repetitions)		/* make this thread safe */
    {
        printf("****Client process\t Action Function\t repetition counter reached max! = %d, creating new WD process\n", current_count);

		ReviveWD();	/* scheduler function? or just regular one-time func? */
		/* SchedulerAddTask(scheduler, SchedulerActionReviveWD, NULL, NULL, 0); */
    }
    
    UNUSED(param);
	return 0;
}




