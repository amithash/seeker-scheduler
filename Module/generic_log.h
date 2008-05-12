/**************************************************************************
 * Copyright 2008 Amithash Prasad                                         *
 * Copyright 2006 Tipp Mosely                                             *
 *                                                                        *
 * This file is part of Seeker                                            *
 *                                                                        *
 * Seeker is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/
#ifndef _GENERIC_LOG_H_
#define _GENERIC_LOG_H_

typedef struct _log_block_t {
	int bytes_logged;
	int bytes_read;
	void *data;
	struct _log_block_t *next;
} log_block_t;

typedef struct {
	spinlock_t log_lock;
	size_t block_size;
	log_block_t *pool;
	log_block_t *reading;
	log_block_t *head, *tail;
} log_t;

log_t *log_create(size_t block_size);
void log_free(log_t *log);
void *log_alloc(log_t *log, size_t bytes);
void log_commit(log_t *log);
int log_read(log_t *log, 
	     struct file *file_ptr, char *buff, size_t count, loff_t *offset);

#endif
