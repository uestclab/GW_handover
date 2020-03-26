#include "priorityQueue.h"

/**
 * init queue
 * @param   queue   queue handler
 */

priorityQueue* init_queue(int size){
    priorityQueue* queue = (priorityQueue*)malloc(sizeof(priorityQueue));
    if(queue == NULL){
        return NULL;
    }
    queue->data = (Queue_item**)malloc(sizeof(Queue_item*)*size);
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
    Queue_item** new_data = (Queue_item**)malloc(sizeof(Queue_item*)*new_size);
    if(new_data == NULL){
        return -1;
    }
    for(int i=0;i<queue->len;i++){
        new_data[i] = queue->data[i];
    }

    free(queue->data);
    queue->data = new_data;
    queue->start = 0;
    queue->size = new_size;

    return 0;
}

/**
 * insert new data into queue
 * @param queue  priority queue handler
 */
 
void upAdjust(priorityQueue* queue){
    int childIndex = queue->len - 1;
    int parentIndex = (childIndex-1)/2;

    Queue_item* temp;
    temp = queue->data[childIndex];
    
    while(childIndex > 0 && temp->level < queue->data[parentIndex]->level){
        queue->data[childIndex] = queue->data[parentIndex];
        childIndex = parentIndex;
        parentIndex = (childIndex-1)/2;
    }

    queue->data[childIndex] = temp;
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

    Queue_item* temp;
    temp = queue->data[parentIndex];
    int childIndex = 2*parentIndex+1;

    // printf("parentIndex = %d , childIndex = %d , temp = %d \n", parentIndex, childIndex, temp);

    while(childIndex < queue->len){
        if(childIndex+1<queue->len && queue->data[childIndex+1]->level<queue->data[childIndex]->level){
            childIndex = childIndex+1;
        }

        if(temp->level<=queue->data[childIndex]->level){
            break;
        }

        queue->data[parentIndex] = queue->data[childIndex];
        parentIndex = childIndex;
        childIndex = 2*parentIndex +1;
    }

    queue->data[parentIndex] = temp;
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
            return -1;
        }
    }

    Queue_item* temp = (Queue_item*)malloc(sizeof(Queue_item));
    temp->data = newData;
    temp->level = level;
    queue->data[queue->len] = temp;
    queue->len++;

    upAdjust(queue);

    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_signal(&queue->wakeup);
    return 0;
}

/**
 * dequeue
 * @param   queue   queue handler
 * @return  dequeue element : min element in queue
 */
void* dePriorityQueue(priorityQueue* queue){
    pthread_mutex_lock(&queue->mutex);
    while(queue->len == 0){
        pthread_cond_wait(&queue->wakeup,&queue->mutex);
    }

    if(queue->len == 0){
        return NULL;
    }

    void* head = queue->data[0]->data;
    int head_level = queue->data[0]->level;
    free(queue->data[0]);
    /* move tail item to head */
    queue->data[0] = queue->data[queue->len-1];
    queue->len = queue->len - 1;

    int level = head_level;
    downAdjust(queue,0);

    pthread_mutex_unlock(&queue->mutex);

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
