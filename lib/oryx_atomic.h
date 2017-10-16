/*
*   rt_atomic.h
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   22 Feb, 2016
*   Func: Lockless Chain
*   Personal.Q
*/

#ifndef __ORYX_ATOMIC_H__
#define __ORYX_ATOMIC_H__

#include "apr_atomic.h"

typedef struct{
#define ATOMIC_SIZE sizeof(atomic_t)
	oryx_int32_t counter;
}atomic_t;

typedef struct{
#define ATOMIC64_SIZE sizeof(atomic64_t)
	int64_t counter;
}atomic64_t;

#define ATOMIC_INIT(i)	{ (i) }

extern oryx_status_t oryx_atomic_init(oryx_pool_t *p);


/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v)	(*(volatile oryx_int32_t *)&(v)->counter)

oryx_int32_t atomic_add (volatile atomic_t *v, oryx_int32_t val);
oryx_int32_t atomic_sub (volatile atomic_t *v, oryx_int32_t val);
oryx_int32_t atomic_inc (volatile atomic_t *v);
oryx_int32_t atomic_dec (volatile atomic_t *v);

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
//#define atomic_set(v, i) (((v)->counter) = (i))
static __oryx_always_inline__
void atomic_set(volatile atomic_t *v, oryx_int32_t val)
{
    v->counter = val;
}

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic64_read(v)	(*(volatile oryx_int64_t *)&(v)->counter)
oryx_int64_t atomic64_add (volatile atomic64_t *v, oryx_int64_t val);
oryx_int64_t atomic64_sub (volatile atomic64_t *v, oryx_int64_t val);
oryx_int64_t atomic64_inc (volatile atomic64_t *v);
oryx_int64_t atomic64_dec (volatile atomic64_t *v);

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
//#define atomic64_set(v, i) (((v)->counter) = (i))
static __oryx_always_inline__
void atomic64_set(volatile atomic64_t *v, oryx_int64_t val)
{
    v->counter = val;
}

#endif
