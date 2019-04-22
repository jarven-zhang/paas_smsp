/*
 * CopyRight(C) elonxu@163.com
 */
#ifndef OPS_CPU_H_
#define OPS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct phy_core_id{
	uint8_t    phy_id;
	uint8_t    core_id;
	uint8_t    used;  /* 0 unused 1 used 2 reserved, 3 permanent reserved */
}phy_core_id;


typedef struct phy_core_info{
	uint8_t all_cores;
	uint8_t reserved_cores;
	uint8_t used_cores;
}phy_core_info;

#ifdef __cplusplus
typedef enum _CoreBindingMode{
	DESC_MODE = 0,
	ASC_MODE
}CoreBindingMode;
#else
#endif

void init_cpu_map();

#ifdef __cplusplus
int get_core_for_probeif(uint32_t* cores, const uint32_t num, CoreBindingMode mode = DESC_MODE);
#else


int get_core_for_probeif(uint32_t* cores, const uint32_t num, int mode);
#endif
//int get_single_core(int * core);

int ops_recede_core(uint8_t lgcore_id);

static inline int ops_setaffinity(int core){
	cpu_set_t  new_mask;
	int ret;

	CPU_ZERO(&new_mask);
	CPU_SET(core, &new_mask);

	ret = sched_setaffinity(0, sizeof(cpu_set_t), &new_mask);
	if (ret == -1){
		printf("warning: could not set cpu affinity [%d]\n", core);
	}

	return ret;
}


void print_cpu_map();

#ifdef __cplusplus
}
#endif


#endif /* OPS_CPU_H_ */
