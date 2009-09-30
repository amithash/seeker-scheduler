/******************************************************************************\
 * FILE: generic.h
 * DESCRIPTION: This is the user space equivalent for seeker.h. I badly needed
 * the ease of using error,warn,info & debug even in user space programs.
 *
 \*****************************************************************************/

/******************************************************************************\
 * Copyright 2009 Amithash Prasad                                              *
 *                                                                             *
 * This file is part of Seeker                                                 *
 *                                                                             *
 * Seeker is free software: you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software   *
 * Foundation, either version 3 of the License, or (at your option) any later  *
 * version.                                                                    *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 * for more details.                                                           *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program. If not, see <http://www.gnu.org/licenses/>.              *
 \*****************************************************************************/

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

