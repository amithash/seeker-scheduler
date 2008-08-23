/****************************************************************************
 * c2dpmu.h part of Seeker under the Dual BSD/GPL Licencing.
 * Author(s): Amithash Prasad
 ***************************************************************************/


#ifndef _PMU_H_
#define _PMU_H_

#include <asm/types.h>

/********** Constants ********************************************************/
#ifdef ARCH_C2D
#	define NUM_COUNTERS 2
#	define NUM_EVTSEL 2
#	define EVTSEL_RESERVED_BITS 0x00200000
#endif

#ifdef ARCH_K8
#	define NUM_COUNTERS 4
#	define NUM_EVTSEL 4
#	define EVTSEL_RESERVED_BITS 0x00200000
#endif

#ifdef ARCH_K10
#	define NUM_COUNTERS 4
#	define NUM_EVTSEL 4
#	define EVTSEL_RESERVED_BITS 0x00200000
#endif
/********** MSR's ************************************************************/
#ifdef ARCH_C2D
#	define EVTSEL0 0x00000186
#	define EVTSEL1 0x00000187
#	define PMC0 0x000000C1
#	define PMC1 0x000000C2
#endif

#if defined(ARCH_K8) || defined(ARCH_K10)
#	define EVTSEL0 0xC0010000
#	define EVTSEL1 0xC0010001
#	define EVTSEL2 0xC0010002
#	define EVTSEL3 0xC0010003
#	define PMC0 0xC0010004
#	define PMC1 0xC0010005
#	define PMC2 0xC0010006
#	define PMC3 0xC0010007
#endif

/********** Structure Definitions ********************************************/
#if defined(ARCH_C2D) || defined(ARCH_K8) || defined(ARCH_K10)
typedef struct {
	u32 ev_select:8;
	u32 ev_mask:8;
	u32 usr_flag:1;
	u32 os_flag:1;
	u32 edge:1;
	u32 pc_flag:1;
	u32 int_flag:1;
	u32 rsvd:1;
	u32 enabled:1;
	u32 inv_flag:1;
	u32 cnt_mask:8;
	u32 addr;
}evtsel_t;

typedef struct {
	u32 low:32;
	u32 high:32;
	u32 addr;
	u32 event;
	u32 mask;
	u32 enabled;
}counter_t;
#endif

/********* Extern Vars *******************************************************/

extern evtsel_t evtsel[NR_CPUS][NUM_COUNTERS];
extern counter_t counters[NR_CPUS][NUM_COUNTERS];
extern char* evtsel_names[NUM_EVTSEL];
extern char* counter_names[NUM_COUNTERS];

/********** Function Prototypes **********************************************/

void pmu_init_msrs(void);

inline u32 evtsel_read(u32 evtsel_num);
inline void evtsel_clear(u32 evtsel_num);
inline void evtsel_write(u32 evtsel_num);

void counter_clear(u32 counter);
void counter_read(void);
u64 get_counter_data(u32 counter, u32 cpu_id);
void counter_disable(int counter);
int counter_enable(u32 event_num, u32 ev_mask, u32 os);

#endif

