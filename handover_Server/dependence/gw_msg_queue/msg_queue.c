#include "msg_queue.h"
#include <sys/ipc.h>
#include <sys/types.h>

//key_t ftok(char *pathname, char proj)

int msg_show_attr(g_msg_queue_para* g_msg_queue){
	int ret = -1;
	struct msqid_ds msg_info;
	ret = msgctl(g_msg_queue->msgid,IPC_STAT,&msg_info);
	if(-1 == ret){
		zlog_info(g_msg_queue->log_handler, "fail get msg queue infomation ");
		return -1;
	}

	return msg_info.msg_qnum;
}

int clearMsgQueue(g_msg_queue_para* g_msg_queue){
	int msg_qnum = msg_show_attr(g_msg_queue);
	zlog_info(g_msg_queue->log_handler," msg_qnum = %d ", msg_qnum);
	while(msg_qnum != 0){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;
		zlog_info(g_msg_queue->log_handler," ----- clearMsgQueue : msg_number = %d , msg_type = %d ", 
		          getData->msg_number , getData->msg_type);
	}
	return 0;
}

g_msg_queue_para* createMsgQueue(const char *pathname, int proj_id, zlog_category_t* handler){
	zlog_info(handler,"Enter createMsgQueue\n");
	g_msg_queue_para* g_msg_queue = (g_msg_queue_para*)malloc(sizeof(struct g_msg_queue_para));
	g_msg_queue->log_handler = handler;
	g_msg_queue->msgid = -1;
	g_msg_queue->seq_id = 1;

	key_t key;
	key = ftok(pathname, proj_id);

  	g_msg_queue->msgid = msgget(key, 0666 | IPC_CREAT);  
	if(g_msg_queue->msgid == -1){
		zlog_error(handler,"msgget failed with error: %d\n", errno); 
		free(g_msg_queue);
		return NULL;  
	}
	g_msg_queue->para_t = newThreadPara();

	return g_msg_queue; 
}

void postMsgQueue(struct msg_st* data, g_msg_queue_para* g_msg_queue){

	data->msg_number = g_msg_queue->seq_id;
	g_msg_queue->seq_id = g_msg_queue->seq_id + 1;

	int send_len = sizeof(struct msg_st) - sizeof(long);

	pthread_mutex_lock(g_msg_queue->para_t->mutex_);
	if(msgsnd(g_msg_queue->msgid, (void*)data, send_len, 0) == -1){  
		zlog_info(g_msg_queue->log_handler,"postMsgQueue : msgsnd failed\n"); 
	}
	pthread_mutex_unlock(g_msg_queue->para_t->mutex_);
}

struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue){
	struct msg_st* out_data = (struct msg_st*)malloc(sizeof(struct msg_st));
	long int msgtype = 0;
	
	int rcv_len = sizeof(struct msg_st) - sizeof(long);

	if(msgrcv(g_msg_queue->msgid, (void*)out_data, rcv_len, msgtype, 0) == -1){  
		zlog_info(g_msg_queue->log_handler, "msgrcv failed with errno: %d\n", errno);
		free(out_data);
		return NULL;   
	}
	return out_data;
}

int delMsgQueue(g_msg_queue_para* g_msg_queue)
{
	destoryThreadPara(g_msg_queue->para_t);
	// g_msg_queue->msgid?
	zlog_info(g_msg_queue->log_handler,"delMsgQueue()");
	free(g_msg_queue);
	return 0;
}








 
