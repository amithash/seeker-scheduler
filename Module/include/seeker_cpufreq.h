/*****************************************************
 * Copyright 2008 Amithash Prasad                    *
 *                                                   *
 * This file is part of Seeker                       *
 *                                                   *
 * Seeker is free software: you can redistribute     *
 * it and/or modify it under the terms of the        *
 * GNU General Public License as published by        *
 * the Free Software Foundation, either version      *
 * 3 of the License, or (at your option) any         *
 * later version.                                    *
 *                                                   *
 * This program is distributed in the hope that      *
 * it will be useful, but WITHOUT ANY WARRANTY;      *
 * without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE. See the GNU General Public License       *
 * for more details.                                 *
 *                                                   *
 * You should have received a copy of the GNU        *
 * General Public License along with this program.   *
 * If not, see <http://www.gnu.org/licenses/>.       *
 *****************************************************/

#ifndef __SCPUFREQ_H_
#define __SCPUFREQ_H_


#define MAX_STATES 8

/* Error codes */
#define ERR_USER_EXISTS -1
#define ERR_USER_MEM_LOW -2
#define ERR_INV_USER -3
#define ERR_INV_CALLBACK -4

struct scpufreq_user{
	int (*inform) (int cpu, int state);
	int user_id;
};


int register_scpufreq_user(struct scpufreq_user *u);
int deregister_scpufreq_user(struct scpufreq_user *u);

int set_freq(unsigned int cpu,unsigned int freq_ind);
int __set_freq(unsigned int cpu,unsigned int freq_ind);
unsigned int get_freq(unsigned int cpu);
int inc_freq(unsigned int cpu);
int dec_freq(unsigned int cpu);
int get_total_states(void);
int get_max_states(int cpu);

#endif

