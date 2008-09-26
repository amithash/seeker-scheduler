
#define MAX_STATES 16

struct freq_info_t{
	unsigned int cpu;
	unsigned int cur_freq;
	unsigned int num_states;
	unsigned int table[MAX_STATES];
};

int set_freq(unsigned int cpu,unsigned int freq_ind);
int inc_freq(unsigned int cpu);
int dec_freq(unsigned int cpu);

