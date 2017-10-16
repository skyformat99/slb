/*
*   rt_task.c
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   1 June, 2015
*   Func: System task control interface
*   Personal.Q
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "oryx.h"
#include "oryx_error.h"
#include "oryx_list.h"
#include "oryx_mallocator.h"
#include "oryx_task.h"
#include "oryx_ipc.h"

/** For thread sync */
sem_t oryx_task_sync_sem;
/** Shared in all proccess, include threads of proccess */
int pshared = 0;

/** All system semphore is initialized in this api */
void rt_sync_init()
{
    sem_init(&oryx_task_sync_sem, pshared, 1);
    /** Add other semphore here */
}

void rt_unsync()
{
    sem_post(&oryx_task_sync_sem);
}

void rt_sync()
{
    sem_wait(&oryx_task_sync_sem);
}

#define	TASK_FLG_INITD		(1 << 0)
struct rt_tasklist_t{
	oryx_thread_mutex_t  lock;
	int count;
	struct list_head	head;
	/** Search with task desc */
	struct hlist_head	hhead;

	int flags;
};

static struct rt_tasklist_t task_list = {
	.flags = 0,
	.count = 0,
	.lock = INIT_MUTEX_VAL,
};

static __oryx_always_inline__
void chk_init(struct rt_tasklist_t *tasklist)
{
	if (!tasklist || (tasklist && !tasklist->flags)) {
		INIT_LIST_HEAD(&tasklist->head);
		INIT_HLIST_HEAD(&tasklist->hhead);
		tasklist->flags |= TASK_FLG_INITD;
		rt_sync_init();
	}
}

void oryx_task_registry(struct oryx_task_t *task)
{
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	if(likely(task)){
		oryx_thread_mutex_lock(&tasklist->lock);
		list_add_tail(&task->list, &tasklist->head);
		tasklist->count ++;
		oryx_thread_mutex_unlock(&tasklist->lock);
	}
	
	return;
}

void oryx_task_deregistry(struct oryx_task_t *task)
{
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	if(likely(task)){
		oryx_thread_mutex_lock(&tasklist->lock);
		list_del(&task->list);
		tasklist->count --;
		oryx_thread_mutex_unlock(&tasklist->lock);

		if(task->recycle == ALLOWED)
			kfree(task);
	}
	
	return;
}


void oryx_task_deregistry_named(const char *desc)
{
	struct oryx_task_t *task = NULL, *p;
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	if(likely(desc)){
		list_for_each_entry_safe(task, p, &tasklist->head, list){
			if (!oryx_strcmp_native(desc, task->name)){
				oryx_task_deregistry(task);
				return;
			}
		}
	}
}

void oryx_task_deregistry_id(oryx_os_thread_t pid)
{
	struct oryx_task_t *task = NULL, *p;
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	list_for_each_entry_safe(task, p, &tasklist->head, list){
		if (pid == task->pid){
			oryx_task_deregistry(task);
			return;
		}
	}
}

struct oryx_task_t  *oryx_task_query_id (oryx_os_thread_t pid)
{
	struct oryx_task_t *task = NULL, *p;
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	oryx_thread_mutex_lock(&tasklist->lock);
	list_for_each_entry_safe(task, p, &tasklist->head, list){
		if (pid == task->pid){
			oryx_thread_mutex_lock(&tasklist->lock);
			return task;
		}
	}
	oryx_thread_mutex_unlock(&tasklist->lock);
	return NULL;
}


/** Thread description should not be same with registered one. */
void oryx_task_spawn(char __oryx_unused__*desc, 
		uint32_t __oryx_unused__ prio, void __oryx_unused__*attr,  void * (*func)(void *), void *arg)
{
	struct oryx_task_t *task = NULL, *p;
	struct rt_tasklist_t *tasklist = &task_list;

	if(unlikely(!desc)){
		return;
	}

	chk_init(tasklist);
	oryx_thread_mutex_lock(&tasklist->lock);
	list_for_each_entry_safe(task, p, &tasklist->head, list){
		if(!oryx_strcmp_native(desc, task->name)){
			oryx_thread_mutex_unlock(&tasklist->lock);
			goto finish;
		}
	}
	oryx_thread_mutex_unlock(&tasklist->lock);

	task = (struct oryx_task_t *)kmalloc(sizeof(struct oryx_task_t), MPF_CLR, -1);
	if(unlikely(!task)){
		goto finish;
	}
	
	task->prio = prio;
	task->routine = func;
	task->argvs = arg;
	INIT_LIST_HEAD(&task->list);
	INIT_HLIST_HEAD(&task->hlist);
	memcpy(task->name, desc, strlen(desc));

	rt_unsync();
	if (!pthread_create(&task->pid, NULL, task->routine, task->argvs) &&
	    		 (!pthread_detach(task->pid))) {
		rt_sync();
		goto registry;
	}
	rt_sync();

	goto finish;

registry:
	oryx_task_registry(task);

finish:
	return;
}

void oryx_task_spawn_quickly(struct oryx_task_t *task)
{
	oryx_task_spawn(task->name, task->prio, DEFAULT_ATTR_VAL, task->routine, task->argvs);
}


static __oryx_always_inline__
void oryx_task_detail(struct oryx_task_t *task, 
                        int __oryx_unused__ flags)
{
	if(likely(task)){
		printf("\t\"%64s\"%20ld\n", task->name, task->pid);
	}
}

void oryx_task_foreach_lineup(struct rt_tasklist_t *tasklist,
				int __oryx_unused__ flags, void (*routine)(struct oryx_task_t *, int))
{
	struct oryx_task_t *task = NULL, *p;

	if(likely(tasklist)){
		list_for_each_entry_safe(task, p, &tasklist->head, list){
		        if(likely(routine))
					routine(task, flags);
		}
	}
}

void oryx_task_detail_foreach()
{
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);
	
	printf("\r\nTask(s) %d Preview\n", 
			tasklist->count);

	printf ("\t%64s%20s\n", "DESCRIPTION", "IDENTI");
		oryx_task_foreach_lineup(tasklist, 0, oryx_task_detail);
	printf("\r\n\r\n");
}

void oryx_task_launch()
{
	struct oryx_task_t *task = NULL, *p;
	struct rt_tasklist_t *tasklist = &task_list;

	chk_init(tasklist);

	list_for_each_entry_safe(task, p, &tasklist->head, list){
		/** check if the task has been started */
		if (task->pid > 0)
			continue;
		
		rt_unsync();
		if (pthread_create(&task->pid, DEFAULT_ATTR_VAL, task->routine, task->argvs)){
			goto finish;
		}

		if (pthread_detach(task->pid)){
			goto finish;
		}
		rt_sync();
	}
	sleep(1);
finish:
	rt_sync();
	oryx_task_detail_foreach();
	return;
}


