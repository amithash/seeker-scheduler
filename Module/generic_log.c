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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "generic_log.h"

#define LOG_POOL_SIZE 4

#define log_lock(pl) spin_lock(&((pl)->log_lock))
#define log_unlock(pl) spin_unlock(&((pl)->log_lock))

static void log_block_free(log_block_t *block);
static log_block_t *log_block_create(log_t *log);
static log_block_t *log_pool_fill(log_t *log);
static log_block_t *log_get_block(log_t *log);
static void log_recycle_block(log_t *log, log_block_t *block);

int log_read(log_t *log, struct file* file_ptr, char *buf, size_t count, loff_t *offset){
	log_block_t *reading, *done_block;

	int len = 0;
  
	if(unlikely(log == NULL || buf == NULL || file_ptr == NULL || count <= 0)) {
		return -1;
	}

	if( !log->reading ) {
		log_lock(log);
		log->reading = log->head;
		log->head = NULL;
		log_unlock(log);
	}
	reading = log->reading;
	if( !reading ) {
		return 0;
	}


	/* while bytes left <= space available */
	while( len + reading->bytes_logged - reading->bytes_read <= count) {
		memcpy(buf + len, ((char*)reading->data) + reading->bytes_read,
		reading->bytes_logged - reading->bytes_read);
		len += reading->bytes_logged - reading->bytes_read;

		/* clear out the block and put it back in the pool if necessary */
		if( reading->next == NULL ) {
			log_recycle_block(log, reading);
			log->reading = NULL;
			return len;
		} else {
			done_block = reading;
			reading = reading->next;
			log->reading = reading;
			log_recycle_block(log, done_block);
		}
	}
	/* bytes left > space available */
	if( reading->bytes_logged - reading->bytes_read > count - len ) {
		memcpy(buf + len, ((char*)reading->data) + reading->bytes_read, count - len);
		reading->bytes_read += count - len;
		len += count - len;
	}
  
	return len;
}

log_t *log_create(size_t block_size){
	log_t *log = kmalloc(sizeof(log_t), GFP_ATOMIC);

	if(unlikely(log == NULL)) {
		return NULL;
	}

	spin_lock_init(&log->log_lock);
	log->block_size = block_size;
	log->pool = NULL;
	log->reading = NULL;

	if(unlikely(!log_pool_fill(log))){
		log_free(log);
		return NULL;
	}
	if(unlikely(!(log->head = log_block_create(log)))){
		log_free(log);
		return NULL;
	}
	log->tail = log->head;

	return log;
}

void log_free(log_t *log){
	log_block_t *tmp_block, *head;

	if(unlikely(log == NULL)) {
		return;
	}
  
	head = log->pool;
	while(head != NULL) {
		tmp_block = head->next;
		log_block_free(head);
		head = tmp_block;
	}
	head = log->head;
	while(head != NULL) {
		tmp_block = head->next;
		log_block_free(head);
		head = tmp_block;
	}
	head = log->reading;
	while(head != NULL) {
		tmp_block = head->next;
		log_block_free(head);
		head = tmp_block;
	}
	kfree(log);
}

void *log_alloc(log_t *log, size_t bytes){    
	void *ret;
  
	if( log == NULL ) {
		return NULL;
	}
	/* log_alloc must be followed by log_commit */
	log_lock(log);

	if( log->head == NULL ) {
		if( (log->head = log_get_block(log)) == NULL ) {
			log_unlock(log);
			return NULL;
		}
		log->tail = log->head;
	}

	if( log->tail->bytes_logged + bytes > log->block_size ) {
		if((log->tail->next = log_get_block(log)) == NULL){
			log_unlock(log);
			return NULL;
		}
		log->tail = log->tail->next;
	} 

	ret = ((char*)log->tail->data) + log->tail->bytes_logged;
	log->tail->bytes_logged += bytes;
	return ret;
}

void log_commit(log_t *log){
	log_unlock(log);
}

static void log_block_free(log_block_t *block){
	if(unlikely(block == NULL)) {
		return;
	}
	vfree(block->data);
	kfree(block);
}

/*
 * Assumes log->lock is NOT held by caller
 */
static void log_recycle_block(log_t *log, log_block_t *block){
	if(unlikely(block == NULL)) {
		return;
	}
	block->bytes_logged = 0;
	block->bytes_read = 0;
	log_lock(log);
	block->next = log->pool;
	log->pool = block;
	log_unlock(log);
}

/*
 * Assumes log->lock is held by caller
 */
static log_block_t *log_get_block(log_t *log){
	log_block_t *ret;
  
	if( !log->pool ) {
		if(unlikely(!log_pool_fill(log))) {
			return NULL;
		}
	}
	ret = log->pool;
	log->pool = log->pool->next;
	ret->bytes_logged = 0;
	ret->bytes_read = 0;
	ret->next = NULL;
	return ret;  
}

/*
 * Assumes log->lock is held by caller
 */
static log_block_t *log_pool_fill(log_t *log){
	int i;
	log_block_t *tmp_block, *tmp_block2;
  
	if(unlikely((tmp_block = log_block_create(log)) == NULL)) {
		return NULL;
	}
	tmp_block2 = tmp_block;
	for(i = 0; i < LOG_POOL_SIZE; i++) {
		if(unlikely((tmp_block->next = log_block_create(log)) == NULL)) {
			return NULL;
		}
		tmp_block = tmp_block->next;
	}
	tmp_block->next = log->pool;
	log->pool = tmp_block2;
	return log->pool;
}

static log_block_t *log_block_create(log_t *log){
	log_block_t *new_block;
  
	new_block = kmalloc(sizeof(log_block_t), GFP_ATOMIC);
  
	if(unlikely(new_block == NULL)) {
		return NULL;
	}
	new_block->bytes_logged = 0;
	new_block->bytes_read = 0;
	new_block->data = vmalloc(log->block_size);

	if(unlikely(new_block->data == NULL)) {
		printk("<0>TIPP new_block->data is NULL.  kernel lights on fire.\n");
		kfree(new_block);
		return NULL;
	}

	new_block->next = NULL;
	return new_block;
}

