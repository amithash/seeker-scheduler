#ifndef _STATS_H_
#define _STATS_H_

/* Some defines to do the math on the returned values. */

#define ADD_LOAD(a,b) ((a) + (b))
#define MUL_LOAD(a,b) (((a) * (b)) >> 3)
#define MUL_LOAD_UINT(a,b) ((a) * (b))
#define DIV_LOAD(a,b) (((a)<<3)/(b))
#define DIV_LOAD_UINT(a,b) ((((a)<<3)/(b))>>3)
#define LOAD_TO_UINT(a) ((a)>>3)
#define UINT_TO_LOAD(a) ((a)<<3)

void init_idle_logger(void);
unsigned int get_cpu_load(int cpu);

#endif
