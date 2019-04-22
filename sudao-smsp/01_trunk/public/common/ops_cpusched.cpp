#include "ops_cpusched.h"
#include <pthread.h>

static int logical_cores = 0;
static int physical_cores = 0;

static phy_core_id   cpu_map[64];
static phy_core_info phy_cores[8];
static int init_cpu_tag = 0;
static pthread_mutex_t core_mutex;

class Init{
 public:
   Init(pthread_mutex_t * lock):_lock(lock){
     pthread_mutex_init(_lock, NULL);
   }
  ~Init(){
    pthread_mutex_destroy(_lock);
  }
  pthread_mutex_t *_lock;
};

static Init initlock(&core_mutex);
	
void init_cpu_map(){

	FILE* fd = fopen("/proc/cpuinfo", "rb");
	char* line = NULL;
	size_t size = 0;
	uint8_t cur_cpu = 0;
	uint8_t cur_phy_id = 0;
	uint8_t cur_core_id = 0;
	char* tmp = NULL;

	memset(cpu_map, 0, sizeof(cpu_map));


	memset(phy_cores, 0, sizeof(phy_cores));

	while (getline(&line, &size, fd) != -1){
		if (strncmp(line, "processor", 9) == 0){
			++logical_cores;
			tmp = strchr(line, ':');
			cur_cpu = atoi(strchr(tmp, ' '));
		}else if (strncmp(line, "physical id", 11) == 0){
			tmp = strchr(line, ':');
			cur_phy_id = atoi(strchr(tmp, ' '));
			if (physical_cores < cur_phy_id){
				physical_cores = cur_phy_id;
			}
		}else if (strncmp(line, "core id", 7) == 0){
			tmp = strchr(line, ':');
			cur_core_id = atoi(strchr(tmp, ' '));
			cpu_map[cur_cpu].phy_id = cur_phy_id;
			cpu_map[cur_cpu].core_id = cur_core_id;
			/*if (phy_cores[cur_phy_id].all_cores == 0){
				cpu_map[cur_cpu].used = 3;
			        phy_cores[cur_phy_id].reserved_cores++;
			}*/
			phy_cores[cur_phy_id].all_cores++;
		}
	}
        physical_cores++;
	if (line){
		free(line);
	}
	fclose(fd);
  cpu_map[0].used = 3;
  phy_cores[cpu_map[0].phy_id].reserved_cores++;
	
  print_cpu_map();
 #if 0 
	for (; i < logical_cores; ++i){
		if (cpu_map[i].used == 3){
			cur_core_id = cpu_map[i].core_id;
			cur_phy_id = cpu_map[i].phy_id;
      int j = i + 1;
			for (; j < logical_cores; j++){
				if (cpu_map[j].phy_id == cur_phy_id && 
					cpu_map[j].core_id == cur_core_id){
					cpu_map[j].used = 2;
					phy_cores[cur_phy_id].reserved_cores++;
					break;
				}
			}
		}
	}
 #endif
}

#if 1
int get_core_for_probeif(uint32_t* cores, const uint32_t num, CoreBindingMode mode){
	int phy_id = 0;
	int left_cores = 0;
	int i = 0;
	uint8_t cur_core_id = 0;
	uint32_t number = num;
		
	//uint8_t* cores2 = cores;
    pthread_mutex_lock(&core_mutex);
    if (init_cpu_tag == 0){
       init_cpu_map();
       init_cpu_tag = 1;
    }
		
	if(DESC_MODE == mode){
		for (i = physical_cores - 1; i >= 0; i--){
			left_cores = phy_cores[i].all_cores - phy_cores[i].reserved_cores - phy_cores[i].used_cores;
			if ((uint32_t)left_cores >=  num ){
				phy_id = i;
				break;
				
			}
		}
	}	
	else{
		for (i = 0; i < physical_cores; i++){
			left_cores = phy_cores[i].all_cores - phy_cores[i].reserved_cores - phy_cores[i].used_cores;
			if ((uint32_t)left_cores >=  num ){
				phy_id = i;
				break;
				
			}
		}
	}
	
	if (i == physical_cores){
		pthread_mutex_unlock(&core_mutex);
		
		return -1;
	}		
	
	printf("on %s, use cpu: %d for binding, cpu count: %d\n", 
				__FUNCTION__, 
				i,
				physical_cores);
	for (i = 0; (i < logical_cores) && number; i++){
		if (cpu_map[i].phy_id == phy_id && cpu_map[i].used == 0){
			cur_core_id = cpu_map[i].core_id;
			cpu_map[i].used = 1;
			number--;
			*cores++ = (i == 0 ? 1 : i);
			phy_cores[phy_id].used_cores++;
							
			printf("on %s:%d, processor id: %d\n", __FUNCTION__, __LINE__, i);
		}
	}	
	pthread_mutex_unlock(&core_mutex);
	
	return 0;
}


#else
int get_core_for_probeif(uint8_t* cores, const uint32_t num){
	int phy_id = 0;
	int left_cores = 0;
	int i = 0;
	uint8_t cur_core_id = 0;
	uint32_t number = num;
		
	//uint8_t* cores2 = cores;
    pthread_mutex_lock(&core_mutex);
    if (init_cpu_tag == 0){
       init_cpu_map();
       init_cpu_tag = 1;
    }
	
	for (; i < physical_cores; i++){
		left_cores = phy_cores[i].all_cores - phy_cores[i].reserved_cores - phy_cores[i].used_cores;
		if (left_cores >=  num ){
			phy_id = i;
			break;

		}
	}

	if (i == physical_cores){
		pthread_mutex_unlock(&core_mutex);
		return -1;
	}

	for (i = 0; (i < logical_cores) && number; i++){
		if (cpu_map[i].phy_id == phy_id && cpu_map[i].used == 0){
			cur_core_id = cpu_map[i].core_id;
			cpu_map[i].used = 1;
			number--;
			
			*cores++ = i;
			//*cores = i;
			//log_msg(2, "on %s:%d, *cores: %d", __FUNCTION__, __LINE__, i);
			log_msg(2, "on %s:%d, *cores: %d", __FUNCTION__, __LINE__, *cores);
			phy_cores[phy_id].used_cores++;
      			int j = i + 1;
			for (; j < logical_cores; j++){
				if (cpu_map[j].phy_id == phy_id && 
					cpu_map[j].core_id == cur_core_id){
					cpu_map[j].used = 1;
					if (number == 0){
						cpu_map[j].used = 2;
						phy_cores[phy_id].reserved_cores++;
						break;
					}
					number--;
					
					*cores++ = j;
					//*cores = j;
					//log_msg(2, "on %s:%d, *cores: %d", __FUNCTION__, __LINE__, j);
					log_msg(2, "on %s:%d, *cores: %d", __FUNCTION__, __LINE__, *cores);
					phy_cores[phy_id].used_cores++;
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&core_mutex);
	
	/* show */
	/*for(i = 0; i < num; i++){
		//uint8_t val = *cores[i];
		
		log_msg(2, "on %s, cores[%d]: %d", __FUNCTION__, i, cores2[i]);
	}*/
	
	return 0;
}

#endif
#if 0
int get_core_for_probeif(uint8_t* cores, const uint32_t num){
  int i = 1;
  uint32_t number = num;

  for (; (i < logical_cores) &&number; ++i){
    if (cpu_map[i].used == 0){
      number--;
			*cores++ = i;
      cpu_map[i].used = 1;
    }
  }
  
  if (number != 0){
    //roll back
    int j = num - number;
    for (i = 0; i < j; i++){
      cpu_map[cores[i]].used = 0;
    }

    printf("not enough cores\n");
    return -1;
  }

  return 0;
}


int get_single_core(int * core){
  int i = 1;
	uint8_t cur_phy_id = 0;
	uint8_t cur_core_id = 0;

  if (init_cpu_tag == 0){
    init_cpu_map();
    init_cpu_tag = 1;
  }

  for (; i < logical_cores; ++i){
		if (cpu_map[i].used == 0){
			*core = i;
			//phy_cores[cpu_map[i].phy_id].used_cores++;
			//phy_cores[cpu_map[i].phy_id].reserved_cores--;
			cpu_map[i].used = 1;
			return 0;
		}
	}

  return -1;
  
}
#endif


#if 0
int get_single_core(int * core){
	int i = 1;
	uint8_t cur_phy_id = 0;
	uint8_t cur_core_id = 0;
	
  pthread_mutex_lock(&core_mutex);
  if (init_cpu_tag == 0){
    init_cpu_map();
    init_cpu_tag = 1;
  }
	
	for (; i < logical_cores; ++i){
		if (cpu_map[i].used == 2){
			*core = i;
			phy_cores[cpu_map[i].phy_id].used_cores++;
			phy_cores[cpu_map[i].phy_id].reserved_cores--;
			cpu_map[i].used = 1;
			pthread_mutex_unlock(&core_mutex);
			return 0;
		}
	}

	for (i = 1; i < logical_cores; ++i){
		if (cpu_map[i].used == 0){
			*core = i;
			cpu_map[i].used = 1;
			cur_phy_id = cpu_map[i].phy_id;
			cur_core_id = cpu_map[i].core_id;
			phy_cores[cur_phy_id].used_cores++;
      			int j = i + 1;
      
			for (; j < logical_cores; j++){
				if (cpu_map[j].phy_id == cur_phy_id && 
					cpu_map[j].core_id == cur_core_id){
					cpu_map[j].used = 2;
					phy_cores[cur_phy_id].reserved_cores++;
					break;
				}
			}
			pthread_mutex_unlock(&core_mutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&core_mutex);
	return -1;
}

#endif 

int ops_recede_core(uint8_t lgcore_id){
  pthread_mutex_lock(&core_mutex);
  if (lgcore_id >= logical_cores){
	pthread_mutex_unlock(&core_mutex);
	return -1;
  }

  cpu_map[lgcore_id].used = 0;
  phy_cores[cpu_map[lgcore_id].phy_id].used_cores--;
  pthread_mutex_unlock(&core_mutex);
  return 0;
  
}



void print_cpu_map(){
	int i = 0;
	int j = 0;
	
	printf("logical_cores count: %d, physical_cores: %d\n", 
								logical_cores,
								physical_cores);
	for( ;i < logical_cores; i++){
		printf("|%d|%d|%d\n", 
									cpu_map[i].phy_id,
									cpu_map[i].core_id,
									cpu_map[i].used);
	}
    printf("********************************\n");
	for ( ;j < physical_cores; j++){
		printf("|%d|%d|%d\n",
									phy_cores[j].all_cores,
									phy_cores[j].reserved_cores,
									phy_cores[j].used_cores);
	}
}

