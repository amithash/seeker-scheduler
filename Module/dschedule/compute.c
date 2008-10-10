#define abs(i) ((i) >= 0 ? (i) : (-1*(i)))

extern int total_states;
extern int max_allowed_states[NR_CPUS];

inline int procs(int hints,int total, int proc);

inline int procs(int hints,int total, int proc)
{
	int i;
	int hp = hints * proc;
	if(hints == 0)
		return 0;
	if(hints == total)
		return proc;
	for(i=1;i<proc;i++){
		if((hp <= i*total) && (hp > (i-1)*total))
			break;
	}
	return i;
}

void choose_layout(int delta)
{
	int cpus = num_online_cpus();
	int hint[MAX_STATES];
	int count = get_hint(hint);
	int i;
	int total = 0;
	int cpus_in_state[MAX_STATES];
	int req_cpus = 0;
	int cpu_state[NR_CPUS] = {-1};
	int dt = delta;

	for(i=0;i<count;i++)
		total += hint[i];
	for(i=0;i<count;i++){
		cpus_in_state[i] = procs(hint[i],total,cpus);
		req_cpus += cpus_in_state[i];
	}
	
	/* FIXME */

	/* Adjust total number of cpus */
	if(req_cpus > cpus){
		/* Drop some cpus.*/

	} else if(req_cpus > cpus){
		/* duplicate some cpus 
		 * XXX Will this ever happen?
		 */
	}

	/* END OF FIXME */


	req_cpus = cpus;

	for(i=0;i<cpus;i++){
		if(cpus_in_state[cur_cpu_state[i]] > 0){
			cpus_in_state[cur_cpu_state[i]]--;
			req_cpus--;
		}
	}
	/* XXX FIXME */
	/* Something tells me that this can be optimized */
	if(req_cpus == 0)
		return;

	for(i=0;i<cpus;i++){
		if(cur_cpu_state[i] > -1)
			continue;
		if(delta == 0)
			return;

		/* Prefer to increase a state by 1 */
		if(cur_cpu_state[i] < count)
			if(cpus_in_state[cur_cpu_state[i]+1] > 0)
				cur_cpu_state[i]++;
				cpus_in_state[cur_cpu_state[i]]--;
				set_freq(i,cur_cpu_state[i]);
				delta--;
				continue;
			}
		}
		/* If not then prefer to decrease a state by 1 */
		if(cur_cpu_state[i] > 0)
			if(cpus_in_state[cur_cpu_state[i]-1] > 0)
				cur_cpu_state[i]--;
				cpus_in_state[cur_cpu_state[i]]--;
				set_freq(i,cur_cpu_state[i]);
				delta--;
				continue;
			}
		}
		/* Else assign any available proc
		 * with the new state as long as 
		 * it honors the remaining delta
		 */
		for(j=0;j<count;j++){
			if(cpus_in_state[j] > 0){
				if(abs(cur_cpu_state[i]-j)<=delta){
					cur_cpu_state[i] = j;
					cpus_in_state[j]--;
					set_freq(i,j);
					break;
				}
			}
		}
	}
}

