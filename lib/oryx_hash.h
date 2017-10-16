/*
*   oryx_hash.h
*   Created by TSIHANG <qh_soledadboy@sina.com>>
*   1 June, 2015
*   Func: Hash API
*   Personal.Q
*/


#ifndef __ORYX_HASH_H__
#define __ORYX_HASH_H__

#define HASH_JEN_MIX(a,b,c)                                             \
    do {                                                                            \
        a -= b; a -= c; a ^= ( c >> 13 );                               \
        b -= c; b -= a; b ^= ( a << 8 );                                \
        c -= a; c -= b; c ^= ( b >> 13 );                               \
        a -= b; a -= c; a ^= ( c >> 12 );                               \
        b -= c; b -= a; b ^= ( a << 16 );                               \
        c -= a; c -= b; c ^= ( b >> 5 );                                \
        a -= b; a -= c; a ^= ( c >> 3 );                                \
        b -= c; b -= a; b ^= ( a << 10 );                               \
        c -= a; c -= b; c ^= ( b >> 15 );                               \
    } while (0)

static __oryx_always_inline__ uint32_t 
hash_data(void *key, int keylen)
{
    unsigned _hj_i, _hj_j, _hj_k;
    char *_hj_key = (char *) key;
    uint32_t hashv = 0xfeedbeef;
    _hj_i = _hj_j = 0x9e3779b9;
    _hj_k = keylen;

    while (_hj_k >= 12)
    {
        _hj_i += (_hj_key[0] + ((unsigned) _hj_key[1] << 8)
                  + ((unsigned) _hj_key[2] << 16)
                  + ((unsigned) _hj_key[3] << 24));
        _hj_j += (_hj_key[4] + ((unsigned) _hj_key[5] << 8)
                  + ((unsigned) _hj_key[6] << 16)
                  + ((unsigned) _hj_key[7] << 24));
        hashv += (_hj_key[8] + ((unsigned) _hj_key[9] << 8)
                  + ((unsigned) _hj_key[10] << 16)
                  + ((unsigned) _hj_key[11] << 24));
        HASH_JEN_MIX(_hj_i, _hj_j, hashv);
        _hj_key += 12;
        _hj_k -= 12;
    }

    hashv += keylen;

    switch (_hj_k)
    {
        case 11:
            hashv += ((unsigned) _hj_key[10] << 24);

        case 10:
            hashv += ((unsigned) _hj_key[9] << 16);

        case 9:
            hashv += ((unsigned) _hj_key[8] << 8);

        case 8:
            _hj_j += ((unsigned) _hj_key[7] << 24);

        case 7:
            _hj_j += ((unsigned) _hj_key[6] << 16);

        case 6:
            _hj_j += ((unsigned) _hj_key[5] << 8);

        case 5:
            _hj_j += _hj_key[4];

        case 4:
            _hj_i += ((unsigned) _hj_key[3] << 24);

        case 3:
            _hj_i += ((unsigned) _hj_key[2] << 16);

        case 2:
            _hj_i += ((unsigned) _hj_key[1] << 8);

        case 1:
            _hj_i += _hj_key[0];
    }

    HASH_JEN_MIX(_hj_i, _hj_j, hashv);
    return hashv;
}

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))
/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

struct oryx_hbucket_t
{
	void *data;
	uint16_t size;
	struct oryx_hbucket_t *next;
};

struct oryx_htable_t
{
	/** hash bucket */
	struct oryx_hbucket_t **array;
	
	/** sizeof hash bucket */
	int array_size;

	/** total of instance stored in bucket, maybe great than array_size in future. */
	int count;

	/** function for create a hash value with the given parameter *v */
	uint32_t (*hash_fn)(struct oryx_htable_t *, void *, uint16_t);

	/** 0: equal, otherwise a value less than zero returned*/
	int (*cmp_fn)(void *, uint16_t, void *, uint16_t);

	/** function for vlaue of oryx_hbucket_t releasing */
	void (*free_fn)(void *);
};


/*
--------------------------------------------------------------------
 This works on all machines.  To be useful, it requires
 -- that the key be an array of uint32_t's, and
 -- that the length be the number of uint32_t's in the key

 The function hashword() is identical to hashlittle() on little-endian
 machines, and identical to hashbig() on big-endian machines,
 except that the length has to be measured in uint32_ts rather than in
 bytes.  hashlittle() is more complicated than hashword() only because
 hashlittle() has to dance around fitting the key bytes into registers.
--------------------------------------------------------------------
*/
static __oryx_always_inline__ uint32_t hashword (
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t        initval)         /* the previous hash, or an arbitrary value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}


static __oryx_always_inline__
uint32_t oryx_hash(char* str, unsigned int len)  
{  
   unsigned int b    = 378551;  
   unsigned int a    = 63689;  
   unsigned int hash = 0;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = hash * a + (*str);  
      a    = a * b;  
   }  
   return hash;  
}  
/* End Of RS Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_js_hash(char* str, unsigned int len)  
{  
   unsigned int hash = 1315423911;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash ^= ((hash << 5) + (*str) + (hash >> 2));  
   }  
   return hash;  
}  
/* End Of JS Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_pjw_hash(char* str, unsigned int len)  
{  
   const unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);  
   const unsigned int ThreeQuarters     = (unsigned int)((BitsInUnsignedInt  * 3) / 4);  
   const unsigned int OneEighth         = (unsigned int)(BitsInUnsignedInt / 8);  
   const unsigned int HighBits          = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);  
   unsigned int hash              = 0;  
   unsigned int test              = 0;  
   unsigned int i                 = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = (hash << OneEighth) + (*str);  
      if((test = hash & HighBits)  != 0)  
      {  
         hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));  
      }  
   }  
   return hash;  
}  
/* End Of  P. J. Weinberger Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_elf_hash(char* str, unsigned int len)  
{  
   unsigned int hash = 0;  
   unsigned int x    = 0;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = (hash << 4) + (*str);  
      if((x = hash & 0xF0000000L) != 0)  
      {  
         hash ^= (x >> 24);  
      }  
      hash &= ~x;  
   }  
   return hash;  
}  
/* End Of ELF Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_bkdr_hash (char* str, unsigned int len)  
{  
   unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */  
   unsigned int hash = 0;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = (hash * seed) + (*str);  
   }  
   return hash;  
}  
/* End Of BKDR Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_sdbm_hash (char* str, unsigned int len)  
{  
   unsigned int hash = 0;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = (*str) + (hash << 6) + (hash << 16) - hash;  
   }  
   return hash;  
}  
/* End Of SDBM Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_djb_hash (char* str, unsigned int len)  
{  
   unsigned int hash = 5381;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = ((hash << 5) + hash) + (*str);  
   }  
   return hash;  
}  
/* End Of DJB Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_dek_hash (char* str, unsigned int len)  
{  
   unsigned int hash = len;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);  
   }  
   return hash;  
}  
/* End Of DEK Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_bp_hash (char* str, unsigned int len)  
{  
   unsigned int hash = 0;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash = hash << 7 ^ (*str);  
   }  
   return hash;  
}  
/* End Of BP Hash Function */  
  
static __oryx_always_inline__
uint32_t oryx_fnv_hash (char* str, unsigned int len)  
{  
   const unsigned int fnv_prime = 0x811C9DC5;  
   unsigned int hash      = 0;  
   unsigned int i         = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash *= fnv_prime;  
      hash ^= (*str);  
   }  
   return hash;  
}  
/* End Of FNV Hash Function */  


static __oryx_always_inline__
uint32_t oryx_fnv1_hash (char* str, unsigned int len)  
{  
   const unsigned int fnv_prime = 16777619;  
   unsigned int hash      = 2166136261L;  
   unsigned int i         = 0;  
   for(i = 0; i < len; str++, i++)  
   {  
      hash *= fnv_prime;  
      hash ^= (*str);  
   }  

   hash += hash << 13;
   hash ^= hash >> 7;
   hash += hash << 3;
   hash ^= hash >> 17;
   hash += hash << 5;
   
   return hash;  
}  

static __oryx_always_inline__
uint32_t oryx_ap_hash (char* str, unsigned int len)  
{  
   unsigned int hash = 0xAAAAAAAA;  
   unsigned int i    = 0;  
   for(i = 0; i < len; str++, i++)  
   {
   #if 0
      hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*str) * (hash >> 3)) :  
                               (~((hash << 11) + (*str) ^ (hash >> 5)));  
   #endif
   }  
   return hash;  
}  
/* End Of AP Hash Function */  


struct oryx_htable_t *
oryx_htable_init(uint32_t size,
		uint32_t (*hash_fn)(struct oryx_htable_t *, void *, uint16_t),
		int (*cmp_fn)(void *, uint16_t, void *, uint16_t),
		void (*free_fn)(void *));
void oryx_htable_destroy(struct oryx_htable_t *ht);
int oryx_htable_add(struct oryx_htable_t *ht,
		void *v, uint16_t s);
int oryx_htable_del(struct oryx_htable_t *ht,
		void *v, uint16_t s);
void *oryx_htable_lookup(struct oryx_htable_t *ht,
		void *v, uint16_t s);


#endif  /* __RV_HASH_H__ */

