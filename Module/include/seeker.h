/******************************************************************************\
 * FILE: seeker.h
 * DESCRIPTION: The global header. Everyone includes this by default.
 * Contains required macros.
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

#ifndef _SEEKER_H_
#define _SEEKER_H_

#include <linux/version.h>

/* Ass holes changed on_each_cpu's declaration.... */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define ON_EACH_CPU(a,b,c,d) on_each_cpu((a),(b),(c),(d))
#else
#define ON_EACH_CPU(a,b,c,d) on_each_cpu((a),(b),(c))
#endif

/* Useful Macros */

/* Error and warn hash defines kern meaning is increased on purpose... */
#define error(str,a...) printk(KERN_EMERG "SEEKER ERROR[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)
#define warn(str,a...) printk(KERN_ERR "SEEKER WARN[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)
#define info(str,a...) printk(KERN_INFO "SEEKER INFO[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)

/* Print Debugging statements only if DEBUG is defined. */
#ifdef DEBUG
#define debug(str,a...) printk(KERN_INFO "SEEKER DEBUG[%s : %s]: " str "\n",__FILE__,__FUNCTION__, ## a)
#else
#define debug(str,a...)
#endif

#define ABS(i) ((i) >= 0 ? (i) : (-1)*((int)(i)))
#define div(a,b) ((b) != 0 ? (((((a)<<1) + (b)))/((b)<<1))  : 0)

#endif
