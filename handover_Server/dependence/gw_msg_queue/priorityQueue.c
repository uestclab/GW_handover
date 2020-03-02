#include "priorityQueue.h"

/**
 * init queue
 * @param   queue   queue handler
 */
priorityQueue* init_queue(int size,zlog_category_t* log_handler){
    priorityQueue* queue = (priorityQueue*)malloc(sizeof(priorityQueue));
    if(queue == NULL){
        return NULL;
    }
    queue->log_handler = log_handler;
    queue->data = (Queue_item*)malloc(sizeof(Queue_item)*size);
    if(queue->data == NULL){
        return NULL;
    }
    queue->len  = 0;
    queue->size = size;
    queue->start = 0;

    if(pthread_mutex_init(&queue->mutex,NULL) == 0 &&
       pthread_cond_init(&queue->wakeup,NULL) == 0){
           return queue;
    }
    free(queue->data);
    free(queue);
    return NULL;
}

int queue_destory(priorityQueue* queue){
    if(pthread_mutex_destroy(&queue->mutex) != 0 || pthread_cond_destroy(&queue->wakeup) != 0){
        return -1;
    }

    if(queue->size != 0){
        free(queue->data);
    }
    free(queue);
    return 0;

}

/**
 * resize queue
 * @param   queue   queue handler
 */
int resize(priorityQueue* queue){
    int new_size = queue->size * 2;
    Queue_item* new_data = (Queue_item*)malloc(sizeof(Queue_item)*new_size);
    if(new_data == NULL){
        return -1;
    }
    for(int i=0;i<queue->len;i++){
        new_data[i].data = queue->data[i].data;
        new_data[i].level = queue->data[i].level;
    }

    free(queue->data);
    queue->data = new_data;
    queue->start = 0;
    queue->size = new_size;

    return 0;
}


void print_array(priorityQueue* queue, int flag){
    if(queue->len == 0){
        zlog_error(queue->log_handler,"empty queue \n");
        return;
    }
    if(flag == 0){
        zlog_info(queue->log_handler,"enqueue : head --------- length = %d ", queue->len);
    }else if(flag == 1){
        zlog_info(queue->log_handler,"dequeue : head ********* length = %d ", queue->len);
    }else if(flag == 2){
        zlog_info(queue->log_handler,"debug ????????? length = %d ", queue->len);
    }
    for(int i=queue->start;i<queue->len;i++){
        if(queue->data[i].level == -1){
            zlog_info(queue->log_handler, "item(level, data address): %d, 0x%x",queue->data[i].level,queue->data[i].data);
        }
    }
    zlog_info(queue->log_handler,"tail of queue \n");
}

/**
 * insert new data into queue
 * @param queue  priority queue handler
 */
 
void upAdjust(priorityQueue* queue){
    int childIndex = queue->len - 1;
    int parentIndex = (childIndex-1)/2;

    //int temp = queue->data[childIndex];
    Queue_item temp;
    temp.data = queue->data[childIndex].data;
    temp.level = queue->data[childIndex].level;
    

    while(childIndex > 0 && temp.level < queue->data[parentIndex].level){
        //queue->data[childIndex] = queue->data[parentIndex];
        queue->data[childIndex].data = queue->data[parentIndex].data;
        queue->data[childIndex].level = queue->data[parentIndex].level;
        childIndex = parentIndex;
        parentIndex = (childIndex-1)/2;
    }

    //queue->data[childIndex] = temp;
    queue->data[childIndex].data = temp.data;
    queue->data[childIndex].level = temp.level;
}
/**
 * downAdjust
 * @param queue    priority queue handler
 * @param parentIndex   parent node which need down adjust
 */
void downAdjust(priorityQueue* queue, int parentIndex){
    if(queue->len == 0 || queue->len == 1){
        return;
    }

    //int temp = queue->data[parentIndex];
    Queue_item temp;
    temp.data = queue->data[parentIndex].data;
    temp.level = queue->data[parentIndex].level;

    int childIndex = 2*parentIndex+1;

    // printf("parentIndex = %d , childIndex = %d , temp = %d \n", parentIndex, childIndex, temp);

    while(childIndex < queue->len){
        if(childIndex+1<queue->len && queue->data[childIndex+1].level<queue->data[childIndex].level){
            childIndex = childIndex+1;
        }

        if(temp.level<=queue->data[childIndex].level){
            break;
        }

        //queue->data[parentIndex] = queue->data[childIndex];
        queue->data[parentIndex].data = queue->data[childIndex].data;
        queue->data[parentIndex].level = queue->data[childIndex].level;
        parentIndex = childIndex;
        childIndex = 2*parentIndex +1;
    }

    //queue->data[parentIndex] =temp;
    queue->data[parentIndex].data = temp.data;
    queue->data[parentIndex].level = temp.level;
}

/**
 * enqueue
 * @param   newData new queue item
 * @param   level   new item level  
 * @param   queue   queue handler
 */
int enPriorityQueue(void* newData, int level, priorityQueue* queue){
    pthread_mutex_lock(&queue->mutex);
    if(queue->size <= queue->len){
        if(resize(queue)<0){
            zlog_error(queue->log_handler, "resize in priority queue failed !\n");
            return -1;
        }
    }
    queue->data[queue->len].level = level;
    queue->data[queue->len].data = newData;
    queue->len++;

    if(level == -1){
        zlog_error(queue->log_handler, "start upAdjust() !\n");
    }
    upAdjust(queue);

    //print_array(queue,0);

    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_signal(&queue->wakeup);
    if(level == -1){
        zlog_error(queue->log_handler, "end enPriorityQueue() !\n");
    }
    return 0;
}

/**
 * dequeue
 * @param   queue   queue handler
 * @return  dequeue element : min element in queue
 */
void* dePriorityQueue(priorityQueue* queue){
    if(-1 == queue->data[0].level){
        zlog_error(queue->log_handler, "dePriorityQueue run !\n");
    }
    pthread_mutex_lock(&queue->mutex);
    while(queue->len == 0){
        if(-1 == queue->data[0].level){
            zlog_error(queue->log_handler, "dePriorityQueue wait pthread_cond_wait() !\n");
        }
        pthread_cond_wait(&queue->wakeup,&queue->mutex);
    }

    if(queue->len == 0){
        return NULL;
    }

    void* head = queue->data[0].data;
    int head_level = queue->data[0].level;
    // zlog_info(queue->log_handler,"de queue in dePriorityQueue!!!! item(level, data address): %d, 0x%x",queue->data[0].level,queue->data[0].data);
    /* move tail item to head */
    //queue->data[0] = queue->data[queue->len-1];
    queue->data[0].data = queue->data[queue->len-1].data;
    queue->data[0].level = queue->data[queue->len-1].level;
    queue->len = queue->len - 1;
    int level = head_level;
    if(level == -1){
        zlog_error(queue->log_handler, "start downAdjust() !\n");
    }
    downAdjust(queue,0);

    //print_array(queue,1);
    pthread_mutex_unlock(&queue->mutex);
    if(level == -1){
        zlog_error(queue->log_handler, "end dePriorityQueue() !\n");
    }

    return head; 
}

#ifdef TEST_MAIN
#define NUMTHREADS 20
// gcc priorityQueue.c -lpthread
typedef struct work_t {
  int a;
  int b;
  int msg_number;
} work_t;

void *do_work(void *arg) {
  // Store queue argument as new variable
  priorityQueue* my_queue = arg;

  // Pop work off of queue, thread blocks here till queue has work
  work_t *work = dePriorityQueue(my_queue);

  // Do work, we're not really doing anything here, but you
  // could
  printf("(%i * %i) = %i ; msg_number = %d \n", work->a, work->b, work->a * work->b , work->msg_number);
  free(work);
  return 0;
}

int main(void){
    int init_number = 1;
    pthread_t threadpool[NUMTHREADS];
    int i;

    // Create new queue
    priorityQueue *my_queue = init_queue(100);

    if (my_queue == NULL) {
        fprintf(stderr, "Cannot creare the queue\n");
        return -1;
    }

    // Create thread pool
    for (i=0; i < NUMTHREADS; i++) {
        // Pass queue to new thread
        if (pthread_create(&threadpool[i], NULL, do_work, my_queue) != 0) {
            fprintf(stderr, "Unable to create worker thread\n");
            return -1;
        }
    }

    // Produce "work" and add it on to the queue
    for (i=0; i < NUMTHREADS; i++) {
        // Allocate an object to push onto queue
        struct work_t* work = (struct work_t*)malloc(sizeof(struct work_t));
        work->a = i+1;
        work->b = 2 *(i+1);
        if(init_number == 5 || init_number == 10){
            work->msg_number = 0;
            init_number++;
        }else{
            work->msg_number = init_number;
            init_number++;
        } 
        // Every time an item is added to the queue, a thread that is
        // Blocked by `tiny_queue_pop` will unblock
        if (enPriorityQueue(work,work->msg_number,my_queue) != 0) {
            fprintf(stderr, "Cannot push an element in the queue\n");
            return -1;
        }
    }


    // Join all the threads
    for (i=0; i < NUMTHREADS; i++) {
        pthread_join(threadpool[i], NULL); // wait for producer to exit
    }

    if (queue_destory(my_queue) != 0) {
        fprintf(stderr, "Cannot destroy the queue, but it doesn't matter becaus");
        fprintf(stderr, "e the program will exit instantly\n");
        return -1;
    } else {
        return 0;
    }
}
#endif
