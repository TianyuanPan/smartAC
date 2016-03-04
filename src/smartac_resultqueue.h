/*
 * smartac_resultqueue.h
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_RESULTQUEUE_H_
#define SMARTAC_RESULTQUEUE_H_


#define LOCK_QUEUE() do { \
	debug(LOG_DEBUG, "Locking queue"); \
	pthread_mutex_lock(&queue_mutex); \
	debug(LOG_DEBUG, "Queue locked"); \
} while (0)


#define UNLOCK_QUEUE() do { \
	debug(LOG_DEBUG, "Unlocking queue"); \
	pthread_mutex_unlock(&queue_mutex); \
	debug(LOG_DEBUG, "Queue unlocked"); \
} while (0)



typedef struct _result_queue t_queue;

typedef struct _result_data t_result;

struct _result_data{
	void *result;
	int   size;
	int   c_size;
};

typedef struct QNode{
	t_result *data;
	struct QNode *next;
}QNode, *QueuePtr;

struct _result_queue{
	QNode *front;
	QNode *rear;
};


t_result *cmdresult_malloc(int size);

void cmdresult_free(t_result *result);

int initial_queue(t_queue *queue);

int destroy_queue(t_queue *queue);

t_result * getout_queue(t_queue *queue);

int insert_queue(t_queue *queue, t_result *elem);





#endif /* SMARTAC_RESULTQUEUE_H_ */
