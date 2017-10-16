/*
*   rt_task.h
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   1 June, 2015
*   Func: System task control interface
*   Personal.Q
*/

#ifndef __ORYX_TASK_H__
#define __ORYX_TASK_H__

struct oryx_task_t {
    /** Not used */
    int module;

#define PID_INVALID -1
	oryx_os_thread_t pid;

#define TASK_NAME_SIZE 128
	char name[TASK_NAME_SIZE + 1];

#define INVALID_CORE (-1)
	/** proc core, kernel schedule if eque INVALID_CORE */
	int core;

#define DEFAULT_ATTR_VAL	NULL
	/** attr of current task */
	//oryx_thread_attr_t *attr;

#define KERNEL_SCHED    0
	/** priority of current task */
	int prio;

	/** argument count */
	int args;

	/** arguments */
	void *argvs;

	/** Executive entry */
	void * (*routine)(void *);

	/** allowed or forbidden */
	int recycle;

	struct list_head   list;
	struct hlist_head	hlist;

};


extern void oryx_task_spawn(char *desc, uint32_t prio, void *attr, void * (*func)(void *), void *arg);
extern void oryx_task_spawn_quickly(struct oryx_task_t *task);

extern void oryx_task_registry(struct oryx_task_t *task);
extern void oryx_task_deregistry(struct oryx_task_t *task);
extern void oryx_task_deregistry_named(const char *desc);
extern void oryx_task_deregistry_id(oryx_os_thread_t pid);
extern struct oryx_task_t  *oryx_task_query_id (oryx_os_thread_t pid);
extern void oryx_task_launch();

#endif
