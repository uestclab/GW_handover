#include <stddef.h>             /* offsetof() */
#include "event_timer.h"


void* process_timer_event(void* args){
	pthread_detach(pthread_self());
    event_timer_t* timer = (event_timer_t*)args;
    while(1){
        ngx_event_expire_timers(timer);
        usleep(1000);
    }
}

/*
 * the event timer rbtree may contain the duplicate keys, however,
 * it should not be a problem, because we use the rbtree to find
 * a minimum timer value only
 */
ngx_int_t
ngx_event_timer_init(event_timer_t** ev_timer)
{
    *ev_timer = (event_timer_t*)malloc(sizeof(event_timer_t));
    event_timer_t* timer = *ev_timer;
    if(timer == NULL){
        return -1;
    }

    pthread_mutex_init(&(timer->mutex),NULL);

    ngx_rbtree_init(&(timer->rbtree), &(timer->sentinel),
                    ngx_rbtree_insert_timer_value);

    int ret = pthread_create(&(timer->pid), NULL, process_timer_event, (void*)(timer));
    if(ret != 0){
		return -1;
    }

    timer->node_num = 0;
    return 0;
}


ngx_msec_t
ngx_event_find_timer(event_timer_t* ev_timer)
{
    ngx_msec_int_t      timer;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    if (ev_timer->rbtree.root == &(ev_timer->sentinel)) {
        return NGX_TIMER_INFINITE;
    }

    root = ev_timer->rbtree.root;
    sentinel = ev_timer->rbtree.sentinel;
    pthread_mutex_lock(&(ev_timer->mutex));
    node = ngx_rbtree_min(root, sentinel);
    pthread_mutex_unlock(&(ev_timer->mutex));
    timer = (ngx_msec_int_t) (node->key - time_update());

    return (ngx_msec_t) (timer > 0 ? timer : 0);
}


void
ngx_event_expire_timers(event_timer_t* ev_timer)
{
    ngx_event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = ev_timer->rbtree.sentinel;
    
    for ( ;; ) {
        root = ev_timer->rbtree.root;

        if (root == sentinel) {
            return;
        }
        pthread_mutex_lock(&(ev_timer->mutex));
        node = ngx_rbtree_min(root, sentinel);
        pthread_mutex_unlock(&(ev_timer->mutex));

        if ((ngx_msec_int_t) (node->key - time_update()) > 0) {
            return;
        }

        ev = (ngx_event_t *) ((char *) node - offsetof(ngx_event_t, timer));

        // ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
        //                "event timer del: %d: %M",
        //                ngx_event_ident(ev->data), ev->timer.key);
        pthread_mutex_lock(&(ev_timer->mutex));
        ngx_rbtree_delete(&(ev_timer->rbtree), &ev->timer);
        ev_timer->node_num --;
        pthread_mutex_unlock(&(ev_timer->mutex));

        ev->handler(ev);
    }
}


ngx_int_t
ngx_event_no_timers_left(event_timer_t* ev_timer)
{
    ngx_event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = ev_timer->rbtree.sentinel;
    root = ev_timer->rbtree.root;

    if (root == sentinel) {
        return 1;
    }

    for (node = ngx_rbtree_min(root, sentinel);
         node;
         node = ngx_rbtree_next(&(ev_timer->rbtree), node))
    {
        ev = (ngx_event_t *) ((char *) node - offsetof(ngx_event_t, timer));
    }

    return 1;
}
