
static int cpu_counters[NR_CPUS][MAX_COUNTERS_PER_CPU];
extern static int log_events[MAX_COUNTERS_PER_CPU];
extern static unsigned int log_ev_masks[MAX_COUNTERS_PER_CPU];
extern static int log_num_events = 0;


/*---------------------------------------------------------------------------*
 * Function: clear_counters
 * Descreption: Clears all counters which can be cleared.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void clear_counters(void){
	int i;
	int cpu = smp_processor_id();

	/* clear the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		counter_clear(cpu_counters[cpu][i]);
	}
	for(i = 0; i < NUM_FIXED_COUNTERS; i++) {
		fcounter_clear(i);
	}

	/* OTHER COUNTERS SHOULD BE CLEARED HERE (If it makes sense that is) */
}

/*---------------------------------------------------------------------------*
 * Function: do_sample
 * Descreption: The mother load! This is the function which takes samples.
 * 		Called everytime an ISR goes off (Timer/PMU/schedule)
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static void do_sample(void) {
	int i,j;
	int cpu;
	log_block_t *pentry;
	unsigned long long now, period;
	unsigned long long last_ts;
	int num = 0;

	read_time_stamp();  
	counter_read();
	fcounter_read();
	if(unlikely(read_temp() == -1)){
		printk(KERN_INFO "Temperature value not valid, retrying\n");
		read_temp();
	}

	/* OTHER COUNTER READ CALLS ARE DONE HERE */

	cpu = smp_processor_id();

	last_ts = get_last_time_stamp(cpu);
	now = get_time_stamp(cpu);
	period = now - last_ts;
	pentry = log_create();
	if(!pentry){
		seeker_sampler_exit_handler();
		goto out;
	}

	pentry->sample.type = SEEKER_SAMPLE;
	pentry->sample.u.seeker_sample.cpu = cpu;
	pentry->sample.u.seeker_sample.pid = cpu_pid[cpu];
	pentry->sample.u.seeker_sample.cycles = period;
	/* log the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		pentry->sample.u.seeker_sample.counters[i] = get_counter_data(cpu_counters[cpu][i], cpu);
		
	}
	/* log the fixed counters */
	for(j=0;j<NUM_FIXED_COUNTERS;j++){
		pentry->sample.u.seeker_sample.counters[i++] = get_fcounter_data(j,cpu);
	}

	pentry->sample.u.seeker_sample.counters[i++] = get_temp(cpu);
	
	/* GETTING DATA FROM OTHER COUNTERS GO HERE */

	log_link(pentry);
  
	out:
	clear_counters();
}




/************************* Initialization functions **************************/


/*---------------------------------------------------------------------------*
 * Function: config_counters
 * Descreption: configure all counters on the current cpu. 
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int config_counters(void){
	int i;
	int cpu = smp_processor_id();

	/* enable and configure the pmu counters */
	for(i = 0; i < log_num_events; i++) {
		cpu_counters[cpu][i] =
		counter_enable(log_events[i], log_ev_masks[i], os_flag);
		if(unlikely(cpu_counters[cpu][i] < 0)) {
			printk("Could not allocate counter for event %d\n", log_events[i]);
			return -1;
		}
		printk("%d: Allocated counter %d for %d:%x\n", cpu, cpu_counters[cpu][i],
								log_events[i], log_ev_masks[i]);
	}

	fcounters_enable(os_flag);
	printk("%d: enabled the fixed counters\n",cpu);

	/* ENABLING AND CONFIGURATION OF COUNTERS ARE DONE HERE. 
	 * NOTE: This is different from initialization. This is where they are enabled 
	 * If such a facility is provided by the hardware */

	clear_counters();

	return 0;
}



/*---------------------------------------------------------------------------*
 * Function: msrs_init
 * Descreption: Initialize msrs if any for each counter used, on all online cpus
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int msrs_init(void){

	/* initialize the pmu counters */
	if(log_num_events > 0){
		if(unlikely(on_each_cpu((void*)pmu_init_msrs,NULL, 1,1) < 0)){
			printk("could not initialize counters !, line %d\n",__LINE__);
			return -1;
  		}                   
	}
	/* initialize the fixed pmu counters */
	if(unlikely(on_each_cpu((void*)fpmu_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize fixed counters !, line %d\n",__LINE__);
		return -1;
  	}                   
	/* initialize the temperature sensors */
	if(unlikely(on_each_cpu((void*)therm_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize the temperature sensors !, line %d\n",__LINE__);
		return -1;
  	}
	/* initialize the time stamp counter */
	if(unlikely(on_each_cpu((void*)tsc_init_msrs,NULL, 1,1) < 0)){
		printk("could not initialize the time stamp counter !, line %d\n",__LINE__);
		return -1;
  	}

	/* OTHER COUNTER's MSRS CAN BE INITIALIZED HERE. */

	// setup the counters modifications -- needed only for counters with information from seeker 
	// that is currently only for the variable pmu counters.
	if(unlikely(on_each_cpu((void*)config_counters,NULL, 1,1) < 0)){
		printk("could not configure counters!, line %d\n",__LINE__);
		return -1;
  	}

	return 0;
}


