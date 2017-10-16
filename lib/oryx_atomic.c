/*
*   oryx_atomic_builtins.h
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   22 Feb, 2016
*   Func: Lockless Chain
*   Personal.Q
*/

#include "oryx.h"
#include "oryx_error.h"
#include "oryx_atomic.h"

oryx_status_t oryx_atomic_init(oryx_pool_t __oryx_unused__ *p)
{
    return ORYX_SUCCESS;
}

oryx_int32_t atomic_add (volatile atomic_t *v, oryx_int32_t val)
{
	return (oryx_int32_t)__sync_add_and_fetch (&v->counter, val);
}

oryx_int32_t atomic_sub (volatile atomic_t *v, oryx_int32_t val)
{
	return (oryx_int32_t)__sync_sub_and_fetch (&v->counter, val);
}

oryx_int32_t atomic_inc (volatile atomic_t *v)
{
	return atomic_add (v, (oryx_int32_t)1);
}

oryx_int32_t atomic_dec (volatile atomic_t *v)
{
	return atomic_sub (v, (oryx_int32_t)1);
}

oryx_int64_t atomic64_add (volatile atomic64_t *v, oryx_int64_t val)
{
	return (oryx_int64_t)__sync_add_and_fetch (&v->counter, val);
}

oryx_int64_t atomic64_sub (volatile atomic64_t *v, oryx_int64_t val)
{
	return (oryx_int64_t)__sync_sub_and_fetch (&v->counter, val);
}

oryx_int64_t atomic64_inc (volatile atomic64_t *v)
{
	return atomic64_add (v, (oryx_int64_t)1);
}

oryx_int64_t atomic64_dec (volatile atomic64_t *v)
{
	return atomic64_sub (v, (oryx_int64_t)1);
}
