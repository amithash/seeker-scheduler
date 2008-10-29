
#ifndef __PMU_PUBLIC_H_
#define __PMU_PUBLIC_H_

#if defined(ARCH_C2D)
#	define NUM_COUNTERS 2
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define NUM_COUNTERS 4
#else
#	define NUM_COUNTERS 0
#error "Architecture not supported."
#endif

#endif
