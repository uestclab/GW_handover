#ifndef _EVENT_TIMER_H_INCLUDED_
#define _EVENT_TIMER_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"
#include <sys/time.h> /* gettimeofday */
#include <unistd.h>   /* usleep */
#include <pthread.h>
//#include "zlog.h"


typedef long int        ngx_int_t;
typedef unsigned long   ngx_uint_t;


#define add_event_timer        ngx_event_add_timer
#define del_event_timer        ngx_event_del_timer // ngx_del_timer


typedef ngx_rbtree_key_t      ngx_msec_t;
typedef ngx_rbtree_key_int_t  ngx_msec_int_t;

typedef struct ngx_event_s           ngx_event_t;
typedef void (*event_handler_pt)(ngx_event_t *ev);

struct ngx_event_s {
    void                *data;   // 
    event_handler_pt    handler; // timeout process handler
    ngx_rbtree_node_t   timer;   // item in rbtree
};

typedef struct event_timer_s         event_timer_t; // timer handler
struct event_timer_s {
    ngx_rbtree_t              rbtree;
    ngx_rbtree_node_t         sentinel;
    pthread_mutex_t           mutex;
    pthread_t                 pid;
    int                       node_num;
};


#define NGX_TIMER_INFINITE  (ngx_msec_t) -1

static ngx_msec_t time_update(void) {

    time_t sec;
    ngx_msec_t msec;
    struct timeval tv;

    (void)gettimeofday(&tv, NULL);

    sec = tv.tv_sec;
    msec = tv.tv_usec / 1000;

    ngx_msec_t ngx_current_msec = (ngx_msec_t)sec * 1000 + msec;

    return ngx_current_msec;
}


ngx_int_t ngx_event_timer_init(event_timer_t** ev_timer);
ngx_msec_t ngx_event_find_timer(event_timer_t* ev_timer);
void ngx_event_expire_timers(event_timer_t* ev_timer);
ngx_int_t ngx_event_no_timers_left(event_timer_t* ev_timer);


static inline void
ngx_event_del_timer(ngx_event_t *ev, event_timer_t* ev_timer)
{
    // ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
    //                "event timer del: %d: %M",
    //                 ngx_event_ident(ev->data), ev->timer.key);
    pthread_mutex_lock(&(ev_timer->mutex));
    ngx_rbtree_delete(&(ev_timer->rbtree), &ev->timer);
    ev_timer->node_num --;
    pthread_mutex_unlock(&(ev_timer->mutex));
}


static inline void
ngx_event_add_timer(ngx_event_t *ev, ngx_msec_t timer, event_timer_t* ev_timer)
{
    ngx_msec_t      key;
    ngx_msec_int_t  diff;

    key = time_update() + timer;

    ev->timer.key = key;

    pthread_mutex_lock(&(ev_timer->mutex));
    ngx_rbtree_insert(&(ev_timer->rbtree), &ev->timer);
    ev_timer->node_num ++;
    pthread_mutex_unlock(&(ev_timer->mutex));
}

#endif /* _EVENT_TIMER_H_INCLUDED_ */
