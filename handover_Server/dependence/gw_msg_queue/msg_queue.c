#include "msg_queue.h"
#include <sys/types.h>

g_msg_queue_para* createMsgQueue(){
	g_msg_queue_para* g_msg_queue = (g_msg_queue_para*)malloc(sizeof(struct g_msg_queue_para));
	g_msg_queue->msgid = -1;
	g_msg_queue->seq_id = 1;

	g_msg_queue->queue = init_queue(QUEUE_SIZE);

	if(g_msg_queue->queue == NULL){
		free(g_msg_queue);
		return NULL;
	}

	g_msg_queue->para_t = newThreadPara();

	return g_msg_queue; 
}

int postMsgQueue(struct msg_st* data, int level, g_msg_queue_para* g_msg_queue){

	pthread_mutex_lock(g_msg_queue->para_t->mutex_);
	if(level == 1){
		data->msg_number = 0;
	}else if(-1 == level){
		data->msg_number = -1;
	}else{
		data->msg_number = g_msg_queue->seq_id;
		g_msg_queue->seq_id = g_msg_queue->seq_id + 1;
		if(g_msg_queue->seq_id == MAX_QUEUE_ID){
			g_msg_queue->seq_id = 1;
		}
	}

	int ret = 0;
	if(enPriorityQueue(data, data->msg_number, g_msg_queue->queue) != 0){
		ret = -1;
	}

	pthread_mutex_unlock(g_msg_queue->para_t->mutex_);

	return ret;
}

struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue){
	struct msg_st* out_data = dePriorityQueue(g_msg_queue->queue);
	return out_data;
}

int delMsgQueue(g_msg_queue_para* g_msg_queue)
{
	destoryThreadPara(g_msg_queue->para_t);

	free(g_msg_queue->queue);
	free(g_msg_queue);
	return 0;
}








 
