#include "oryx.h"
#include "oryx_hash.h"
#include "slb.h"

int boundClusterHash = 0;
#define MAX_MACHINES	5
#define MAX_INJECT_DATA	1000000
static struct cluster_node_t node[MAX_INJECT_DATA];

static __oryx_always_inline__ uint32_t
next_rand_ (uint32_t *p)
{
	uint32_t seed = *p;

	seed = 1103515145 * seed + 12345;
	*p = seed;

	return seed;
}

static __oryx_always_inline__
uint32_t hash_ (char* str, unsigned int __oryx_unused__ len)  
{  
   	unsigned int hash      = 2166136261L;  
	unsigned int rnd;
	struct in_addr ipint;

	inet_aton(str, &ipint);
	
   	rnd = (((ipint.s_addr >> 15) & 0x7FFF) ^\
				((ipint.s_addr  & 0x7FFF)) ^\
				(ipint.s_addr  >> 30));

    hash = next_rand_(&rnd) % CLUSTER_HASH_TABLE_SIZE;
	
   return hash;
}

static __oryx_always_inline__
uint32_t hash2_ (char* str, unsigned int __oryx_unused__ len)  
{  
   	//return oryx_fnv1_hash (str, len);
	return hash_(str, len);
}

static void ipaddr_generate (char *ipv4)
{
#define itoa(a,s,t)\
	sprintf (s, "%d", a);

	int a = 0, b = 0, c = 0, d = 0;
	char aa[4], bb[4], cc[4], dd[4];
	
	a = rand () % 256;
	b = rand () % 256;
	c = rand () % 256;
	d = rand () % 256;

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

static __oryx_always_inline__ 
void msisdn_generate (char *val)
{
#define itoa(a,s,t)\
	sprintf (s, "%d", a);

	int c = 86, ndc = 138, a, offset = 8;
	char aa[4], cc[4], dd[4];

	itoa (c, cc, 10);
	itoa (ndc, dd, 10);
	
	strcpy (val, cc);
	strcpy (val, dd);
	while (offset -- ) {
		a = rand () % 9;
		itoa (a, aa, 10);
		strcat (val, aa);
	}
	//printf ("%s\n", val);
}

static int
cluster_build_hash_table (struct cluster_t *c, int *outval)
{
	int left = CLUSTER_HASH_TABLE_SIZE;
	int m    = 0;
	int i    = 0;
	unsigned int rnd[CLUSTER_MAX_MACHINES];
	unsigned int mach[CLUSTER_MAX_MACHINES];
	int total = CLUSTER_HASH_TABLE_SIZE;
	volatile int mine_vnode = 0;
	
	for (i = 0; i < C_NS(c); i++) {
		int mine = total / (C_NS(c) - i);
		mine_vnode = mach[i]  = mine;
		//printf ("port_%d = %d  ", i, mine);
		total -= mine;
	}
	//printf ("\n");


	// seed the random number generator with the ip address
	// do a little xor folding to get it into 15 bits
	//
	
	for (m   = 0; m < C_NS(c); m++) {

		/**
		rnd[m] = (((c->machines[m]->ipint >> 15) & 0x7FFF) ^\
				(c->machines[m]->ipint & 0x7FFF)) ^\
				(c->machines[m]->ipint >> 30);
		*/
		rnd[m] = (((N_IPVAL(c->machines[m]) >> 15) & 0x7FFF) ^\
				((N_IPVAL(c->machines[m]) & 0x7FFF)) ^\
				(N_IPVAL(c->machines[m]) >> 30));
	}

	// Initialize the table to "empty"
	//
	for (i = 0; i < CLUSTER_HASH_TABLE_SIZE; i++)
		c->hash_table[i] = 255;

	// Until we have hit every element of the table, give each
	// machine a chance to select it's favorites.
	//
	m = 0;
	while (left) {

		if (!mach[m] && boundClusterHash) {
		      m = (m + 1) % C_NS(c);
		      continue;
		}
			
		do {
		    i = next_rand_(&rnd[m]) % CLUSTER_HASH_TABLE_SIZE;
		} while (c->hash_table[i] != 255);
		
		mach[m]--;
		c->hash_table[i] = m;
		left--;
		m = (m + 1) % C_NS(c);
	}

	if (outval)
		*outval = mine_vnode;
#if 0
	/** Dump hash table. */
	struct cluster_node_t * s;
	for (i = 0; i < CLUSTER_HASH_TABLE_SIZE; i ++) {
		s = c->machines[c->hash_table[i]];
		printf ("%s ", s->madesc);
	}
	printf ("\n");
#endif

	return 0;
}

int vns = 0;
struct cluster_t *
cluster_add_node (struct cluster_t *cc, struct cluster_node_t *m)
{
	// Build a new cluster configuration with the new machine.
	// Machines are stored in ip sorted order.
	
	/* Find the place to insert this new machine */
	int i = 0, j = 0;
  
	for (i = 0; i < C_NS(cc); i++) {
		if (N_IPVAL(cc->machines[i]) > N_IPVAL(m))
			break;
	}

	/* Move the other machines out of the way */
	for (j = cc->n_machines - 1; j >= i; j--)
		cc->machines[j + 1] = cc->machines[j];

	/* Insert it */
	cc->machines[i] = m;
	cc->n_machines++;


	cluster_build_hash_table (cc, &m->valid_vns);
	cc->total_vns += m->valid_vns;
	

	vns += m->valid_vns;
	printf ("Add port %s, %d, %d\n", m->madesc, m->valid_vns, vns);
	
	return cc;
}

struct cluster_t *
cluster_remove_node (struct cluster_t *cc, struct cluster_node_t *m)
{
	/* remove m and move others down */

	for (int i = 0; i < cc->n_machines - 1; i++)
		if (m == cc->machines[i])
			m = cc->machines[i] = cc->machines[i + 1];

	cc->n_machines--;

	cluster_build_hash_table (cc, NULL);

	return cc;
}

struct cluster_node_t *
cluster_lookup_node (struct cluster_t *cc, unsigned int hash)
{
	return cc->machines[cc->hash_table[hash % CLUSTER_HASH_TABLE_SIZE]];
}

void copy_ (struct cluster_node_t *n, char *key)
{
	memset (n, 0, sizeof (struct cluster_node_t));
	inet_aton (key, &N_IPINT(n));
}

void test_data_prepare ()
{
#define RANDOM_INPUT_DATA

	char key [32] = {0};
	ipaddr_generate (key);
	
	/** Backup node */
	for (int i = 0; i < MAX_INJECT_DATA; i ++) {		

#ifdef RANDOM_INPUT_DATA
		ipaddr_generate (key);
#endif
		copy_ (&node[i], key);
	}
}

void install_machine (struct cluster_t *cc, int machines)
{

	struct cluster_node_t *m;
	char key [32] = {0};
	char desc[32] = {0};
	
	memset ((void *)cc, 0, sizeof (struct cluster_t));

	cc->n_machines = 0;
	for (int i = 0; i < CLUSTER_HASH_TABLE_SIZE; i ++) {
		cc->hash_table[i] = 255;
	}

	for (int i = 0; i < machines; i ++) {

		m = (struct cluster_node_t *)malloc (sizeof (struct cluster_node_t));
		sprintf (desc, "Port_%d", i);

	alloc:
		ipaddr_generate (key);

		/** Is there a node with the same IP */
		for (int x = 0; x < cc->n_machines; x ++) {
			struct cluster_node_t *n;
			n = cc->machines[x];
			if (!strcmp (inet_ntoa(n->ipint), key)) {
				goto alloc;
			}
		}
		
		m->madesc = strdup (desc);
		inet_aton (key, &m->ipint);
		
		cluster_add_node (cc, m);
	}
	
}

int ave = MAX_INJECT_DATA/MAX_MACHINES;
int max_node_stat, min_node_stat;
void update_state (struct cluster_node_t *n) {
	if ((int)N_HITS(n) > max_node_stat)
		max_node_stat = N_HITS(n);
	if ((int)N_HITS(n) < min_node_stat)
		min_node_stat = N_HITS(n);
}

void dump_hit_ratio (struct cluster_t *cc)
{

	int i;
	struct cluster_node_t *n;

	for (i = 0; i < cc->n_machines; i ++) {
		n = cc->machines[i];
		update_state (n);
	}
	
	printf ("\n\n\nTotal %15d(%-5d vns) machines, %d hits, ave %d, max (%d %.2f%s) min (%d %.2f%s)\n", 
		C_NS(cc), C_VNS(cc), C_HITS(cc),
		ave, 
		max_node_stat, (float)(max_node_stat - ave)/ave * 100, "%",
		min_node_stat, (float)(ave - min_node_stat)/ave * 100, "%");

	printf ("%15s%16s%8s%15s%15s\n", "MACHINE", "IPADDR", "VNS", "HIT", "RATIO");

	for (i = 0; i < C_NS(cc); i ++) {
		n = cc->machines[i];
		printf ("%15s%16s%8d%15d%15.2f%s\n", 
					n->madesc, inet_ntoa (n->ipint), n->valid_vns, N_HITS(n), (float)N_HITS(n)/C_HITS(cc)* 100, "%");
	}
	
#ifdef DEBUG_MAP
	for (i = 0; i < cc->n_machines; i ++) {
		n = cc->machines[i];
		printf ("%s\n", n->h);
	}
#endif

}

/** Add a node */
void check_hit_ratio6 (struct cluster_t *cc)
{

	int i = 0;
	uint32_t changes = 0;
	struct cluster_t *newcc;
	struct cluster_node_t *temp, *n, *n1;
	
	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));

	/** Backup CC to newcc */
	memcpy (newcc, cc, sizeof (struct cluster_t));
	newcc->total_hit_times = 0;
	/** Clear sent history */
	for (i = 0; i < newcc->n_machines; i ++) {
		n = newcc->machines[i];
		free (n->h);
		n->l = 0;
		n->hits = 0;
	}

	temp = (struct cluster_node_t *)malloc (sizeof (struct cluster_node_t));
	char desc[32] = {0};
	sprintf (desc, "Port_%d", 100);
	temp->madesc = strdup (desc);
	inet_aton ("2.2.2.2", &temp->ipint);
	
	cluster_add_node (newcc, temp);
	changes = 0;
	
			/** test with mapping data */
			for (i = 0; i < MAX_INJECT_DATA; i ++) {		

				char *key = NULL;
				n = &node[i];
				key = inet_ntoa(N_IPINT(n));
				
				/** 2nd, lookup node for a new object if no such object. */
				n = cluster_lookup_node (newcc, hash2_(key, strlen(key)));
				if (n) {
					N_HITS_INC(n);
					newcc->total_hit_times ++;
			#ifdef DEBUG_MAP		
					if (!n->h) {
						n->h = malloc (10240);
						memset (n->h, 0, 10240);
					}
					if (n->h) {
						if (n->l == 0)
							n->l += sprintf (n->h + n->l, "%s: ", n->madesc);
						n->l += sprintf (n->h + n->l, "%s  ", key);
					}
			#endif
				}
				
				n1 = cluster_lookup_node (cc, hash2_(key, strlen(key)));
				if (strcmp (n->madesc, n1->madesc))
					changes ++;
			}

	dump_hit_ratio (newcc);
	printf ("%s changes (%u, %.2f%s)\n", inet_ntoa (temp->ipint), changes, (float)changes/MAX_INJECT_DATA* 100, "%");
	
	free (newcc);
}


void check_hit_ratio5 (struct cluster_t *cc)
{
	uint32_t changes = 0;
	struct cluster_t *newcc;
	char *colur = CONSOLE_PRINT_CLOR_LWHITE;
	
	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));

	printf ("Checking... %s\n", __func__);
	
	for (int i = 0; i < cc->n_machines; i ++) {

		memcpy (newcc, cc, sizeof (struct cluster_t));
		cluster_remove_node (newcc, cc->machines[i]);
		changes = 0;

		struct cluster_node_t *n;
		for (int i = 0; i < MAX_INJECT_DATA; i ++) {		

			n = &node[i];
			char *key = inet_ntoa(N_IPINT(n));

			if (cluster_lookup_node(cc, hash_(key, strlen(key))) != cluster_lookup_node(newcc, hash_(key, strlen(key))))
				changes ++;
		};

		if (changes <= 1899)
			colur = CONSOLE_PRINT_CLOR_LWHITE;
		if ((changes > 1900)  && (changes <= 1999))
			colur = CONSOLE_PRINT_CLOR_LYELLOW;
		if (changes >= 2000)
			colur = CONSOLE_PRINT_CLOR_LRED;
		
		printf ("Removing ...(%s)\n Changes (%-8u%s%-4.2f%s"CONSOLE_PRINT_CLOR_FIN")\n", 
			cc->machines[i]->madesc, 
			changes, colur, (float)changes/MAX_INJECT_DATA * 100, "%");
	}

	free (newcc);
}

void check_hit_ratio4 (struct cluster_t *cc)
{
	uint32_t changes = 0;
	struct cluster_t *newcc;
	
	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));
	
	for (int i = 0; i < cc->n_machines; i ++) {

		memcpy (newcc, cc, sizeof (struct cluster_t));
		cluster_remove_node (newcc, cc->machines[i]);
		changes = 0;

		for (int j = 0; j < CLUSTER_HASH_TABLE_SIZE; j ++) {

				if (cluster_lookup_node(cc, j) != cluster_lookup_node(newcc, j)) {
					changes ++;
				}
		}

		printf ("%s changes (%u, %.2f%s)\n", inet_ntoa (cc->machines[i]->ipint), changes, (float)changes/CLUSTER_HASH_TABLE_SIZE * 100, "%");
	}

}

/** Check miss ratio while nodes removed one by one from cluster. */
void check_hit_ratio3 (struct cluster_t *cc)
{
	uint32_t changes = 0;
	struct cluster_t *newcc;
	
	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));
	
	for (int i = 0; i < cc->n_machines; i ++) {

		memcpy (newcc, cc, sizeof (struct cluster_t));
		cluster_remove_node (newcc, cc->machines[i]);
		changes = 0;

		for (int j = 0; j < CLUSTER_HASH_TABLE_SIZE; j ++) {
			if (cc->hash_table[j] != i) {
				if (cc->hash_table[j] < i) {
					if (cc->hash_table[j] != newcc->hash_table[j]) {
						changes ++;
					}
				}
				else {
					if (cc->hash_table[j] != (newcc->hash_table [j] + 1)) {
						changes ++;
					}
				}
			}
		}

		printf ("%s changes (%u, %.2f%s)\n", inet_ntoa (cc->machines[i]->ipint), changes, (float)changes/MAX_INJECT_DATA* 100, "%");
	}
	
}

/** Check miss ratio while remove one node from cluster. */
void check_hit_ratio2 (struct cluster_t *cc)
{

	uint32_t changes = 0;
	struct cluster_t *newcc;
	int i = 0;
	int removed_node = 0;
	struct cluster_node_t *n, *n1;
	
	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));

	/** Backup CC to newcc */
	memcpy (newcc, cc, sizeof (struct cluster_t));
	newcc->total_hit_times = 0;
	/** Clear sent history */
	for (i = 0; i < newcc->n_machines; i ++) {
		n = newcc->machines[i];
		free (n->h);
		n->l = 0;
		n->hits = 0;
	}

	for (removed_node = 0; removed_node < 1; removed_node ++) {

		/** Remove a node from newcc */
		cluster_remove_node (newcc, cc->machines[removed_node]);
		changes = 0;

		/** test with mapping data */
		for (i = 0; i < MAX_INJECT_DATA; i ++) {		

			char *key = NULL;
			n = &node[i];
			key = inet_ntoa(N_IPINT(n));
			
			/** 2nd, lookup node for a new object if no such object. */
			n = cluster_lookup_node (newcc, hash_(key, strlen(key)));
			if (n) {
				N_HITS_INC(n);
				newcc->total_hit_times ++;
		#ifdef DEBUG_MAP		
				if (!n->h) {
					n->h = malloc (10240);
					memset (n->h, 0, 10240);
				}
				if (n->h) {
					if (n->l == 0)
						n->l += sprintf (n->h + n->l, "%s: ", n->madesc);
					n->l += sprintf (n->h + n->l, "%s  ", key);
				}
		#endif
			}
			
			n1 = cluster_lookup_node (cc, hash_(key, strlen(key)));
			if (strcmp (n->madesc, n1->madesc))
				changes ++;
		}

		//dump_hit_ratio (newcc);
		printf ("%s changes (%u, %.2f%s)\n", inet_ntoa (cc->machines[removed_node]->ipint), changes, (float)changes/MAX_INJECT_DATA* 100, "%");
	}
}
/** Check miss ratio while remove one node from cluster. */
void check_hit_ratio1 (struct cluster_t *cc)
{
	uint32_t changes = 0;
	struct cluster_t *newcc;
	int i = 0;

	newcc = (struct cluster_t *)malloc(sizeof(struct cluster_t));

	memcpy (newcc, cc, sizeof (struct cluster_t));
	{
		cluster_remove_node (newcc, cc->machines[i]);
		changes = 0;

		for (int j = 0; j < CLUSTER_HASH_TABLE_SIZE; j ++) {
			if (cc->hash_table[j] != i) {
				if (cc->hash_table[j] < i) {
					if (cc->hash_table[j] != newcc->hash_table[j]) {
						changes ++;
					}
				}
				else {
					if (cc->hash_table[j] != (newcc->hash_table [j] + 1)) {
						changes ++;
					}
				}
			}
		}

		printf ("%s changes (%u, %.2f%s)\n", inet_ntoa (cc->machines[i]->ipint), changes, (float)changes/MAX_INJECT_DATA* 100, "%");
	}
	
}


void check_hit_ratio (struct cluster_t *cc)
{

	struct cluster_node_t *n;
	//char key [32] = {0};
	int i;

	max_node_stat = 0;
	min_node_stat = MAX_INJECT_DATA;
	
	for (i = 0; i < MAX_INJECT_DATA; i ++) {		

		char *key = NULL;
		n = &node[i];
		key = inet_ntoa(N_IPINT(n));
		
		/** 2nd, lookup node for a new object if no such object. */
		n = cluster_lookup_node (cc, hash2_(key, strlen(key)));
		if (n) {
			N_HITS_INC(n);
			cc->total_hit_times ++;

	#ifdef DEBUG_MAP		
			if (!n->h) {
				n->h = malloc (10240);
				memset (n->h, 0, 10240);
			}
			if (n->h) {
				if (n->l == 0)
					n->l += sprintf (n->h + n->l, "%s: ", n->madesc);
				n->l += sprintf (n->h + n->l, "%s  ", key);
			}
	#endif
		}
	};

	dump_hit_ratio (cc);
}

int main ()
{

	int machines = MAX_MACHINES;
	struct cluster_t cc;


	test_data_prepare ();
	
	/** install machines for cluster. */
	install_machine (&cc, machines);

	/** testing consistent hash capacity of balance */
	check_hit_ratio (&cc);

	/** testing consistent hash  capacity of consistency */
	check_hit_ratio2 (&cc);

	//check_hit_ratio5 (&cc);

	//check_hit_ratio6 (&cc);
	
	return 0;
}
