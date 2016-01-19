/*
 * smartac_resultqueue.c
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */

#include <pthread.h>
#include <syslog.h>

#include "smartac_safe.h"
#include "smartac_debug.h"
#include "smartac_common.h"
#include "smartac_resultqueue.h"



pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void cmdresult_free(t_result *result)
{
	free(result->result);
	free(result);
}


t_result *cmdresult_malloc(int buf_size)
{
	t_result *buf = NULL;
	buf = (t_result *)safe_malloc(sizeof(t_result));
	if(!buf)
		return NULL;
	if(buf_size > MAX_CMD_OUT_BUF + 1)
		buf_size = MAX_CMD_OUT_BUF + 1;
	buf->result = safe_malloc(++buf_size * sizeof(char));
	if(!buf->result){
		free(buf);
		return NULL;
	}
	buf->size = buf_size;
	buf->c_size = 0;
	memset(buf->result, 0, buf->size);
	return buf;
}



int initial_queue(t_queue *queue)
{
	LOCK_QUEUE();

	QueuePtr p;
	p = (QueuePtr)safe_malloc(sizeof(QNode));
	if(!p)
		return -1;
	queue->front = p;
	queue->rear = p;

	queue->front-> next = NULL;
	queue->rear->next = NULL;//FIX ME?

	UNLOCK_QUEUE();
	return 0;
}



int destroy_queue(t_queue *queue)
{
	LOCK_QUEUE();
	while (queue->front){
		queue->rear = queue->front->next;

		cmdresult_free(queue->front->data);

		free(queue->front);
		queue->front = queue->rear;
	}
	UNLOCK_QUEUE();
	return 0;
}


t_result *getout_queue(t_queue *queue)
{
	QNode *p;
	t_result *res;

	LOCK_QUEUE();
	if (queue->front == queue->rear){
		UNLOCK_QUEUE();
		return NULL;
	}

	p = queue->front->next;

	res = cmdresult_malloc(p->data->size);
	if(!res){
		UNLOCK_QUEUE();
		return NULL;
	}
	memcpy(res->result, p->data->result, p->data->size);
	res->c_size = p->data->c_size;

	queue->front->next = p->next;
	if (queue->rear == p)
		queue->rear = queue->front;

	cmdresult_free(p->data);
	free(p);
	UNLOCK_QUEUE();
	return res;
}



int insert_queue(t_queue *queue, t_result *elem)
{
	QNode *p;
	p = (QueuePtr)safe_malloc(sizeof(QNode));
	if(!p)
		return -1;
	p->data = elem;
	p->next = NULL;

	LOCK_QUEUE();

	queue->rear->next = p;
	queue->rear = p;

	UNLOCK_QUEUE();

	return 0;
}



