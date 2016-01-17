/*
 * smartac_resultqueue.h
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_RESULTQUEUE_H_
#define SMARTAC_RESULTQUEUE_H_


typedef struct _result_queue t_queue;

typedef struct _result_data t_result;

struct _result_data{
	void *data;
	int   size;
	int   c_size;
};

struct QNode{
	t_result data;
	struct QNode *netx;
};

struct _result_queue{
	struct QNode *front;
	struct QNode *rear;
};

t_result *cmdresutl_malloc(int size);

void cmdresutl_free(t_result *result);

int initial_queue(t_queue *queue);

int destroy_queue(t_queue *queue);

int get_queue_head(t_queue *queue, t_result *elem);

int insert_queue_tail(t_queue *queue, t_result *elem);




#endif /* SMARTAC_RESULTQUEUE_H_ */
