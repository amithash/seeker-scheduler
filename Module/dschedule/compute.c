extern int hint[TOTAL_HINTS];
extern int total_states;
extern int max_allowed_states[NR_CPUS];

float bins[NR_CPUS];

void init_cpus(void)
{
	int cpus = num_online_cpus();
	int i;
	for(i=0;i<cpus;i++){
		bins[i] = (float)i / (float)cpus;
}

int compute_num_cpus(int ht,int tot)
{
	float total = (float)tot;
	float hint = (float)ht;
	int i;
	int cpus = num_online_cpus();

	/* Hint is 0, no cpus must exist 
	 * in this state
	 */
	if(ht == 0)
		return 0;

	/* Total is 0. return -1 so 
	 * appropiate action is performed.
	 */
	if(tot == 0)
		return -1;
	/* Hint is now the ratio of demand */
	hint = hint / total;

	if(hint <= bins[0])
		return 1;

	/* Find the bin it belongs to. */
	for(i=1;i<cpus;i++){
		if(hint > bins[i-1] && hint <= bins[i])
			break;
	}
	return i+1;
}

