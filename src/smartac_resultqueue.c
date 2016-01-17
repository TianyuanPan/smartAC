/*
 * smartac_resultqueue.c
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */


#include "smartac_safe.h"
#include "smartac_common.h"
#include "smartac_resultqueue.h"


int initial_queue(t_queue *queue)
{
	queue->front = queue->rear = (struct QNode *)safe_malloc(sizeof(struct QNode));
	if(!queue->front)
		return -1;
	queue->front->netx = NULL;
	return 0;
}



int destroy_queue(t_queue *queue)
{
	while (queue->front){
		queue->rear = queue->front->netx;
		free(queue->front);
		queue->front = queue->rear;
	}
	return 0;
}


int  get_queue_head(t_queue *queue, t_result *elem)
{
	struct QNode *p;

	if (queue->front == queue->rear)
		return -1;
	p = queue->front->netx;
	if(elem)
	    *elem = p->data;
	queue->front->netx = p->netx;
	if (queue->rear == p)
		queue->rear = queue->front;

	free(p);

	return 0;
}



int insert_queue_tail(t_queue *queue, t_result *elem)
{
	struct QNode *p;
	p = (struct QNode *)safe_malloc(sizeof(struct QNode));
	if(!p)
		return -1;
	p->data = *elem;
	p->netx = NULL;
	queue->rear->netx = p;
	queue->rear = p;
	return 0;
}



void cmdresutl_free(t_result *result)
{
	free(result->data);
	free(result);
}


t_result *cmdresutl_malloc(int buf_size)
{
	t_result *buf = NULL;
	buf = (t_result *)safe_malloc(sizeof(t_result));
	if(!buf)
		return NULL;
	if(buf_size > MAX_CMD_OUT_BUF)
		buf_size = MAX_CMD_OUT_BUF;
	buf->data = safe_malloc(buf_size * sizeof(char));
	if(!buf->data){
		free(buf);
		return NULL;
	}
	buf->size = buf_size;
	buf->c_size = 0;
	return buf;
}
