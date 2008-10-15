
#define MAX_STATES 16

struct freq_info_t{
	unsigned int cpu;
	unsigned int cur_freq;
	unsigned int num_states;
	unsigned int table[MAX_STATES];
};

int set_freq(unsigned int cpu,unsigned int freq_ind);
unsigned int get_freq(unsigned int cpu);
int inc_freq(unsigned int cpu);
int dec_freq(unsigned int cpu);
int get_total_states(void);
int get_max_states(int cpu);

