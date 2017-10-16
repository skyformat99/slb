#ifndef __ORYX_MALLOCATOR_H__
#define __ORYX_MALLOCATOR_H__

/** memory flags */
#define MPF_NOFLGS  (0 << 0)
#define MPF_CLR         (1 << 0)      /** Clear it after allocated */

extern void rt_kmalloc(void **ptr, size_t s);

extern void *kcalloc(int c, int s, 
                int __oryx_unused__ flags, 
                int __oryx_unused__ node);

extern void *kcalloc(int c, int s, 
                        int flags, 
                        int node);

extern void *kmalloc(int s, 
                        int flags, 
                        int __oryx_unused__ node);

extern void *krealloc(void *sp,  int s, 
                        int flags, 
                        int __oryx_unused__ node);

extern void kfree(void *p);

#endif

