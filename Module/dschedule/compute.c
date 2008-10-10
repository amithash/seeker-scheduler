
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
		} else {
			cur_cpu_state[i] = -1;
		}
	}
	/* XXX FIXME */
	/* Something tells me that this can be optimized */
	if(req_cpus == 0)
		return;

	for(i=0;i<cpus;i++){
		if(cur_cpu_state[i] > -1)
			continue;

		/* Prefer to increase a state by 1 */
		if(cur_cpu_state[i] < count)
			if(cpus_in_state[cur_cpu_state[i]+1] > 0)
				cur_cpu_state[i]++;
				cpus_in_state[cur_cpu_state[i]]--;
				set_freq(i,cur_cpu_state[i]);
				continue;
			}
		}
		/* If not then prefer to decrease a state by 1 */
		if(cur_cpu_state[i] > 0)
			if(cpus_in_state[cur_cpu_state[i]-1] > 0)
				cur_cpu_state[i]--;
				cpus_in_state[cur_cpu_state[i]]--;
				set_freq(i,cur_cpu_state[i]);
				continue;
			}
		}
		/* Else assign any avaliable state to it */
		for(j=0;j<count;j++){
			if(cpus_in_state[j] > 0){
				cur_cpu_state[i] = j;
				cpus_in_state[j]--;
				set_freq(i,j);
				break;
			}
		}
	}
}

