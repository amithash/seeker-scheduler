
#ifndef __FPMU_PUBLIC_H_
#define __FPMU_PUBLIC_H_

#if defined(ARCH_C2D)
#	define NUM_FIXED_COUNTERS 3
#elif defined(ARCH_K8) || defined(ARCH_K10)
#	define NUM_FIXED_COUNTERS 0
#else
#error "Architecture not supported."
#endif

#endif
