#ifndef __ILRD_SCHEDULER_H__
#define __ILRD_SCHEDULER_H__

#include <stddef.h> /* size_t */

#include "uid.h" /*ilrd_uid_t*/

/* will be in the scheduler.c file
sturct scheduler 
{
    pqueue_t list;
};
*/


/* seperate task.c file 
struct task
{
    scheduler_action_func_t scheduler_action_func;
    scheduler_clean_func_t scheduler_clean_func;
    void *action_params;
    time_t time_to_run
    size_t interval;
    ilrd_uid_t uid;
};
*/

/*in task.h file
typedef struct task ilrd_task_t; 
*/

/*---------------------*/
typedef sturct scheduler scheduler_t;

/*
*scheduler_action_func_t
description: typedef to function pointer that executes the task. 
params: (void * - input data) 
return Value: 0 for success and stay in scheduler, 
              1 for success and leave scheduler
             -1 for failiure.
*/
typedef int (*scheduler_action_func_t)(void *);

/*
CreateScheduler
description: creates a new scheduler container
input: void
return: a pointer to a scheduler, if failed NULL
time complexity: O(1)
space complexity: O(1)
*/
scheduler_t *SchedulerCreate(void);

/*
SchedulerDestroy
description: destroy the scheduler
input: pointer to scheduler
return: nothing
time complexity: O(n)
space complexity: O(1)
*/
void SchedulerDestroy(scheduler_t *scheduler);

/*
SchedulerAdd
description: adds new event to the scheduler according to its priority.
input: pointer to scheduler, pointer to the event data
return: success status: 0 success, otherwise failure
time complexity: O(n)
space complexity: O(1)
*/
ilrd_uid_t SchedulerAddTask(scheduler_t *scheduler,
                            scheduler_action_func_t action_func,
                            scheduler_clean_func_t clean_func,
                            void *action_param,
                            size_t time_interval);

/*
SchedulerRemove
description: remove the task with the same as uid as the parameter
input: pointer to scheduler, pointer to the event data
return: status 0 find and removed task, non zero if nothing to remove.
time complexity: O(n)
space complexity: O(1)
*/
int SchedulerRemove(scheduler_t *scheduler, ilrd_uid_t task_uid);

/*
SchedulerRun
description: Runs the tasks according to priority. Runs until stopped, failed, or no tasks to run.
input: pointer to scheduler.
return: status:
    -1 for run error 
    0 for out of tasks
    1 for stop
time complexity: O(n)
space complexity: O(1)
*/
int SchedulerRun(scheduler_t *scheduler);

/*
SchedulerStop
description: stop SchedulerRun function
input: pointer to scheduler,
return: the exit status of the handler function
time complexity: O(1)
space complexity: O(1)
*/
int SchedulerStop(scheduler_t *scheduler);

/*
SchedulerSize
description: return the count of events in the scheduler container
input: pointer to scheduler,
return: size_t number of events in the scheduler
time complexity: O(n)
space complexity: O(1)
*/
size_t SchedulerSize(const scheduler_t *scheduler);

/*
SchedulerIsEmpty
description: return the scheduler container have events  
input: pointer to scheduler,
return: int ,0 - there is events in the scheduler, otherwise there isnt events in the scheduler
time complexity: O(1)
space complexity: O(1)
*/
int SchedulerIsEmpty(const scheduler_t *scheduler);

/*
SchedulerClear
description: remove all the events whithout handling them and clean up afterwards
input: pointer to scheduler,
return: void
time complexity: O(n)
space complexity: O(1)
*/
void SchedulerClear(scheduler_t *scheduler);

#endif /*__SCHEDULER_H__ */a
