#ifndef _GENERIC_H_
#define _GENERIC_H_


/* Error and warn hash defines kern meaning is increased on purpose... */
#ifdef error
#undef error
#endif
#define error(str,a...) fprintf(stderr,"SEEKER ERROR[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)

#ifdef warn
#undef warn
#endif
#define warn(str,a...) fprintf(stderr,"SEEKER WARN[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)

#ifdef info
#undef info
#endif
#define info(str,a...) fprintf(stderr,"SEEKER INFO[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)

/* Print Debugging statements only if DEBUG is defined. */
#ifdef debug
#undef debug
#endif
#ifdef DEBUG
#	define debug(str,a...) fprintf(stderr,"SEEKER DEBUG[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)
#else
#	define debug(str,a...) do{;}while(0);
#endif


#endif

