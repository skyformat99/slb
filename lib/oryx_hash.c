/*
*   rt_hash.c
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   1 June, 2015
*   Func: hash_fn API
*   Personal.Q
*/

#include "oryx.h"
#include "oryx_mallocator.h"
#include "oryx_hash.h"

#define DEFAULT_HASH_CHAIN_SIZE	100
#define TIMES 10000000

static __oryx_always_inline__
void func_free (void *v)
{
	printf ("%s\n", (char *)v);
	//kfree (v);
}

static uint32_t
func_hval (struct oryx_htable_t *ht,
		void *v, uint16_t s) 
{
     uint8_t *d = (uint8_t *)v;
     uint32_t i;
     uint32_t hv = 0;

     for (i = 0; i < s; i++) {
         if (i == 0)      hv += (((uint32_t)*d++));
         else if (i == 1) hv += (((uint32_t)*d++) * s);
         else             hv *= (((uint32_t)*d++) * i) + s + i;
     }

     hv *= s;
     hv %= ht->array_size;
     
     return hv;
}

static int
func_cmp (void *v1, 
		uint16_t s1,
		void *v2,
		uint16_t s2)
{
	int xret = 0;

	if (!v1 || !v2 ||s1 != s2 ||
		memcmp(v1, v2, s2))
		xret = 1;
	
	return xret;
}


#if 0

struct oryx_htable_t *
oryx_htable_init(uint32_t size,
		uint32_t (*hash_fn)(struct oryx_htable_t *, void *, uint16_t),
		int (*cmp_fn)(void *, uint16_t, void *, uint16_t),
		void (*free_fn)(void *))
{
	struct oryx_htable_t *ht = NULL;

	if (size == 0)
		goto finish;

	ht = kmalloc(sizeof (struct oryx_htable_t), MPF_CLR, -1);

	ht->array_size = size;
	ht->hash_fn = hash_fn ? hash_fn : func_hval;
	ht->free_fn = free_fn ? free_fn : func_free;
	ht->cmp_fn = cmp_fn ? cmp_fn : func_cmp;

	ht->array = kmalloc((ht->array_size * sizeof (struct oryx_hbucket_t *)), MPF_CLR, -1);
	if (!ht->array)
		goto error;

       goto finish;
       
error:
	if (ht){
		if (ht->array)
			kfree (ht->array);
		kfree (ht);
	}

finish:
	return ht;
}

void
oryx_htable_destroy(struct oryx_htable_t *ht)
{
	struct oryx_hbucket_t *tb, *next = NULL;
	int hv = 0;

	if (!ht)
		return;
	
	for (hv = 0; hv < ht->array_size; hv ++){
		tb = ht->array[hv];
		while (tb){
			next = tb->next;
			if (ht->free_fn)
				ht->free_fn(tb->value);
			kfree(tb);
			tb = next;
		}
	}

	if (ht->array)
		kfree (ht->array);
	kfree (ht);
	return;
}

int
oryx_htable_add(struct oryx_htable_t *ht,
		void *v, uint16_t s)
{
	uint32_t hv = 0;
	int xret = -1;
	struct oryx_hbucket_t *tb;

	if (!ht ||
		!v || !s)
		goto finish;

	hv = ht->hash_fn(ht, v, s);
	tb = kmalloc(sizeof (struct oryx_hbucket_t), MPF_CLR, -1);
	if (!tb)
		goto finish;

	memset (tb, 0, sizeof (struct oryx_hbucket_t));
	tb->value = v;
	tb->size = s;
	tb->next = NULL;

       
	if (!ht->array[hv]){
		ht->array[hv] = tb;	
	}else{
		tb->next = ht->array[hv];
		ht->array[hv] = tb;
	}

	ht->count ++;
	xret = 0;

finish:
//printf("hv=%u, %s\n", hv, (char *)v);
	return xret;
}

int
oryx_htable_del(struct oryx_htable_t *ht,
		void *v, uint16_t s
/** Delete a val: 0 success, a retval less than zero returned if failure*/)
{
	uint32_t hv = 0;
	int xret = -1;

	if (!ht ||
		!v || !s)
		goto finish;

	hv = ht->hash_fn(ht, v, s);
	if (!ht->array[hv])
		goto finish;

	/** 1st, only one bucket  */
	if (!ht->array[hv]->next)
	{
		if (ht->free_fn)
			ht->free_fn(ht->array[hv]->value);
		kfree(ht->array[hv]);
		ht->array[hv] = NULL;
	}

	struct oryx_hbucket_t *tb = ht->array[hv], *prev = NULL;
	do {
		if (ht->cmp_fn){
			if (!ht->cmp_fn(tb->value, tb->size, v, s)){
				if (!prev)/** as root */
					ht->array[hv] = tb->next;
				else
					prev->next = tb->next;

				if (ht->free_fn)
					ht->free_fn(tb->value);
				kfree(tb);
				ht->count --;
				xret = 0;
				goto finish;
			}

			prev = tb;
			tb = tb->next;
		}
	}while (tb);

finish:
	return xret;
}

void *
oryx_htable_lookup(struct oryx_htable_t *ht,
		void *v, uint16_t s)
{

	uint32_t hv;
	void *xret = NULL;

	if (!ht ||
		!v || !s) 
		goto finish;

	hv = ht->hash_fn(ht, v, s);
	if (!ht->array[hv])
		goto finish;

	struct oryx_hbucket_t *tb = ht->array[hv];
	do{
		if (ht->cmp_fn){
			if (!ht->cmp_fn(tb->value, tb->size, v, s)){
				xret = tb->value;
				goto finish;
			}
		}
		tb = tb->next;
	}while(tb);

finish:
	return xret;
}

#else

uint32_t HashTableGenericHash(struct oryx_htable_t *ht, void *data, uint16_t datalen)
{
     uint8_t *d = (uint8_t *)data;
     int i;
     uint32_t hash = 0;

     for (i = 0; i < datalen; i++) {
         if (i == 0)      hash += (((uint32_t)*d++));
         else if (i == 1) hash += (((uint32_t)*d++) * datalen);
         else             hash *= (((uint32_t)*d++) * i) + datalen + i;
     }

     hash *= datalen;
     hash %= ht->array_size;
     return hash;
}

int HashTableDefaultCompare(void *data1, uint16_t len1, void *data2, uint16_t len2)
{
    if (len1 != len2)
        return 0;

    if (memcmp(data1,data2,len1) != 0)
        return 0;

    return 1;
}

struct oryx_htable_t* oryx_htable_init (uint32_t size, 
	uint32_t (*hash_fn)(struct oryx_htable_t *, void *, uint16_t), 
	int (*cmp_fn)(void *, uint16_t, void *, uint16_t), 
	void (*free_fn)(void *)) {

    struct oryx_htable_t *ht = NULL;

    if (size == 0) {
        goto error;
    }

    if (hash_fn == NULL) {
        //printf("ERROR: HashTableInit no hash_fn function\n");
        goto error;
    }

    /* setup the filter */
    ht = kmalloc(sizeof(struct oryx_htable_t), MPF_CLR, -1);
    if (unlikely(ht == NULL))
    goto error;
    memset(ht,0,sizeof(struct oryx_htable_t));
    ht->array_size = size;
    ht->hash_fn = hash_fn;
    ht->free_fn = free_fn;

    if (cmp_fn != NULL)
        ht->cmp_fn = cmp_fn;
    else
        ht->cmp_fn = HashTableDefaultCompare;

    /* setup the bitarray */
    ht->array = kmalloc(ht->array_size * sizeof(struct oryx_hbucket_t *), MPF_CLR, -1);
    if (ht->array == NULL)
        goto error;
    memset(ht->array,0,ht->array_size * sizeof(struct oryx_hbucket_t *));

    return ht;

error:
    if (ht != NULL) {
        if (ht->array != NULL)
            kfree(ht->array);

        kfree(ht);
    }
    return NULL;
}

void oryx_htable_destroy(struct oryx_htable_t *ht)
{
    int i = 0;

    if (ht == NULL)
        return;

    /* free the buckets */
    for (i = 0; i < ht->array_size; i++) {
        struct oryx_hbucket_t *hashbucket = ht->array[i];
        while (hashbucket != NULL) {
            struct oryx_hbucket_t *next_hashbucket = hashbucket->next;
            if (ht->free_fn != NULL)
                ht->free_fn(hashbucket->data);
            kfree(hashbucket);
            hashbucket = next_hashbucket;
        }
    }

    /* free the arrray */
    if (ht->array != NULL)
        kfree(ht->array);

    kfree(ht);
}

void oryx_htable_print(struct oryx_htable_t *ht)
{
    printf("\n----------- hash_fn Table Stats ------------\n");
    printf("Buckets:               %" PRIu32 "\n", ht->array_size);
    printf("hash_fn function pointer: %p\n", ht->hash_fn);
    printf("-----------------------------------------\n");
}

int oryx_htable_add(struct oryx_htable_t *ht, void *data, uint16_t datalen)
{
    if (ht == NULL || data == NULL)
        return -1;

    uint32_t hash = ht->hash_fn(ht, data, datalen);

    struct oryx_hbucket_t *hb = kmalloc(sizeof(struct oryx_hbucket_t), MPF_CLR, -1);
    if (unlikely(hb == NULL))
        goto error;
    memset(hb, 0, sizeof(struct oryx_hbucket_t));
    hb->data = data;
    hb->size = datalen;
    hb->next = NULL;

    if (ht->array[hash] == NULL) {
        ht->array[hash] = hb;
    } else {
        hb->next = ht->array[hash];
        ht->array[hash] = hb;
    }

#ifdef UNITTESTS
    ht->count++;
#endif

    return 0;

error:
    return -1;
}

int oryx_htable_del(struct oryx_htable_t *ht, void *data, uint16_t datalen)
{
    uint32_t hash = ht->hash_fn(ht, data, datalen);

    if (ht->array[hash] == NULL) {
        return -1;
    }

    if (ht->array[hash]->next == NULL) {
        if (ht->free_fn != NULL)
            ht->free_fn(ht->array[hash]->data);
        kfree(ht->array[hash]);
        ht->array[hash] = NULL;
        return 0;
    }

    struct oryx_hbucket_t *hashbucket = ht->array[hash], *prev_hashbucket = NULL;
    do {
        if (ht->cmp_fn(hashbucket->data,hashbucket->size,data,datalen) == 1) {
            if (prev_hashbucket == NULL) {
                /* root bucket */
                ht->array[hash] = hashbucket->next;
            } else {
                /* child bucket */
                prev_hashbucket->next = hashbucket->next;
            }

            /* remove this */
            if (ht->free_fn != NULL)
                ht->free_fn(hashbucket->data);
            kfree(hashbucket);
            return 0;
        }

        prev_hashbucket = hashbucket;
        hashbucket = hashbucket->next;
    } while (hashbucket != NULL);

    return -1;
}

void *oryx_htable_lookup(struct oryx_htable_t *ht, void *data, uint16_t datalen)
{
    uint32_t hash = 0;

    if (ht == NULL)
        return NULL;

    hash = ht->hash_fn(ht, data, datalen);

    if (ht->array[hash] == NULL)
        return NULL;

    struct oryx_hbucket_t *hashbucket = ht->array[hash];
    do {
		printf ("%s. %s\n", (char *)data, (char *)hashbucket->data);
        if (ht->cmp_fn(hashbucket->data, hashbucket->size, data, datalen) == 1)
            return hashbucket->data;

        hashbucket = hashbucket->next;
    } while (hashbucket != NULL);

    return NULL;
}

#endif

/** Test hash lookup */
char lookup_val[64] = {0};
uint32_t rand_;

/** Random value generator. */
static __oryx_always_inline__
uint32_t next_rand_ (uint32_t *p)
{
	uint32_t seed = *p;

	seed = 1103515145 * seed + 12345;
	*p = seed;

	return seed;
}

/** A random IPv4 address generator.*/
static __oryx_always_inline__
void ipaddr_generate (char *ipv4)
{
#define itoa(a,s,t)\
	sprintf (s, "%d", a);

	int a = 0, b = 0, c = 0, d = 0;
	char aa[4], bb[4], cc[4], dd[4];
	
	a = next_rand_ (&rand_) % 256;
	b = next_rand_ (&rand_) % 256;
	c = next_rand_ (&rand_) % 256;
	d = next_rand_ (&rand_) % 256;

	itoa (a, aa, 10);
	itoa (b, bb, 10);
	itoa (c, cc, 10);
	itoa (d, dd, 10);

	strcpy (ipv4, aa);
	strcat (ipv4, ".");
	strcat (ipv4, bb);
	strcat (ipv4, ".");
	strcat (ipv4, cc);
	strcat (ipv4, ".");
	strcat (ipv4, dd);
	
}


int
rt_hash_table_test (void) 
{
	int i, r;
	int result = 0;
	char val[4096] = {0};
	
	struct oryx_htable_t *ht = oryx_htable_init(DEFAULT_HASH_CHAIN_SIZE, 
			func_hval, func_cmp, func_free);
	
	if (ht == NULL)
		goto end;

	for (i = 0; i < TIMES; i ++) {

		memset (val, 0, 4096);
		ipaddr_generate (val);
		
		r = oryx_htable_add(ht, val, strlen(val));
		if (r != 0) {
			goto end;
		}
		
		void *rp = oryx_htable_lookup(ht, val, strlen(val));
		if (rp == NULL) {
			goto end;
		}
		
		r = oryx_htable_del (ht, val, strlen(val));
		if (r != 0)
			goto end;
	}

	/* all is good! */
	result = 1;
end:
	oryx_htable_destroy(ht);
	return result;
}

