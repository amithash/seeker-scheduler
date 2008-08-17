#include "io.h"


/************************* FILE OPS FUNCTIONS *******************************/

/*---------------------------------------------------------------------------*
 * Function: seeker_sample_log_read
 * Descreption: This is the "read" function. So daemons can read the buffer.
 * Input Parameters: Same params as a generic read function.
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static ssize_t seeker_sample_log_read(struct file *file_ptr, char __user *buf, 
			      size_t count, loff_t *offset){
	return log_read(seeker_sample_log, file_ptr, buf, count, offset);
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_open
 * Descreption: Seeker's file 'open' handle Creates and initializes the kernel 
 * 		buffer and enables PMU interrupts if required, and starts sampling
 * 		by setting dev_open to 1.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
int seeker_sample_open(struct inode *in, struct file * f){

	int i,retval=0;
	seeker_sampler_entry_t *pentry;

	if(unlikely((seeker_sample_log = log_create(4096 * sizeof(seeker_sample_t))) == NULL)) {
		return -1;
	}

	if(unlikely((pentry = log_alloc(seeker_sample_log, sizeof(seeker_sampler_entry_t))) == NULL)){
		return -1;
	}

	pentry->type = SAMPLE_DEF;
	pentry->u.seeker_sample_def.num_counters = log_num_events + NUM_FIXED_COUNTERS + NUM_EXTRA_COUNTERS;
	for(i = 0; i < log_num_events; i++) {
		pentry->u.seeker_sample_def.counters[i] = log_events[i];
		pentry->u.seeker_sample_def.masks[i] = log_ev_masks[i];
	}

	/* add definations for the fixed counters:
	* 0x01:0x00 = FIXED COUNTER 0; 
	* 0x02:0x00 = FIXED COUNTER 1; 
	* 0x03:0x00 = FIXED COUNTER 2; 
	* 0x04:0x00 = TEMPERATURE
	*/
	pentry->u.seeker_sample_def.counters[i] = 0x01;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x02;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x03;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;
	pentry->u.seeker_sample_def.counters[i] = 0x04;
	pentry->u.seeker_sample_def.masks[i] = 0x00;
	i++;

	log_commit(seeker_sample_log);

	retval = generic_open(in,f);
	dev_open = 1;
	/* Enable and configure interrupts on each cpu */
	#ifdef LOCAL_PMU_VECTOR
	if(pmu_intr >= 0){
		if(on_each_cpu((void *)configure_enable_interrupts,NULL,1,1) < 0){
			printk("Could not configure interrupts on all cpu's");
		}
	}
	#endif
	return retval;
}


/*---------------------------------------------------------------------------*
 * Function: seeker_sample_close
 * Descreption: seeker's file close handle. Frees all log buffers, disables 
 * 		interrupts if enabled, and stops any sampling by setting
 * 		dev_open to 0.
 * Input Parameters: None
 * Output Parameters: None
 *---------------------------------------------------------------------------*/
static int seeker_sample_close(struct inode *in, struct file *f){
	int retval=0;
	dev_open = 0;
	/* Disable interrupts on each cpu. */
	#ifdef LOCAL_PMU_VECTOR
	if(pmu_intr >= 0){
		if(unlikely(on_each_cpu((void *)configure_disable_interrupts,NULL,1,1) < 0)){
			printk("Oops... Could not disable interrupts on all cpu's");
		}
	}
	#endif
	if(likely(seeker_sample_log != NULL)) {
		log_free(seeker_sample_log);
		seeker_sample_log = NULL;
	}
	retval = generic_close(in,f);
	return retval;
}

