#ifndef __ORYX_CLUSTER_H__
#define __ORYX_CLUSTER_H__

#define CLUSTER_MAX_MACHINES 256

// less than 1% disparity at 255 machines, 32707 is prime less than 2^15
#define CLUSTER_HASH_TABLE_SIZE 32707

struct cluster_node_t {

	struct in_addr ipint;

	char *madesc;	/** Discripter for this real node instance, 
						and should be uniquel. */
	int valid_vns;	/** Validate virtual nodes within this real node instance. */

	uint32_t hits;	/** For hit testing. */

#define MAX_WEIGHT_VAL		255
	int weight;
	int current_weight;
	int effective_weight;


	char *h;	/** for test */
	int l;
};
#define N_HITS_INC(n) ((n)->hits ++)
#define N_HITS(n) ((n)->hits)
#define N_IPINT(n) ((n)->ipint)
#define N_IPVAL(n) ((n)->ipint.s_addr)

struct cluster_t {

	void (*build_map_table) (struct cluster_t *);
	
	struct cluster_node_t *machines[CLUSTER_MAX_MACHINES];
	int n_machines;			/** Total count of real node instance. */
	unsigned char hash_table[CLUSTER_HASH_TABLE_SIZE];

	int total_vns;				/** Total count of virtual node. */
				
	int total_hit_times;
};

#define C_NS(c) ((c)->n_machines)
#define C_HITS(c) ((c)->total_hit_times)
#define C_VNS(c) ((c)->total_vns)
#define C_NODE_AT(c, i) ((c)->machines[i % CLUSTER_MAX_MACHINES])
#define C_HTABLE_AT(c, i) ((c)->hash_table[i % CLUSTER_HASH_TABLE_SIZE])


#endif
