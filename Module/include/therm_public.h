
#ifndef __THERM_PUBLIC_H_
#define __THERM_PUBLIC_H_

#if defined(ARCH_C2D)
#	define THERM_SUPPORTED 1
#elif defined(ARCH_K8)
#	define THERM_SUPPORTED 0
#elif defined(ARCH_K10)
#	define THERM_SUPPORTED 1
#else
#	error "Architecture not supported"
#endif

#endif
