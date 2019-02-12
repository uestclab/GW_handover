#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event-config.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <signal.h>
#include <vector>

#include <fstream>
#include <glog/logging.h>

#include "common.h"
#include "baseStation.h"
#include "manager.h"
#include "gw_utility.h"

using namespace std;
vector<struct event*> eventSet; // add to manage and delete event to end event-loop

class LogAssitant
{
public:
    LogAssitant()
    {
        google::InitGoogleLogging("handoverServer");
        FLAGS_colorlogtostderr = true;
        FLAGS_alsologtostderr = true;
        google::SetLogDestination(google::INFO, "./");
    }
    ~LogAssitant()
    {
        google::ShutdownGoogleLogging();
    }
};

static void
signal_callback(evutil_socket_t fd, short event, void *pArg){
    LOG(INFO) << "signal_callback: got signal ";
    
    //Manager* pManager = (Manager*)pArg;
    int sizeSet = eventSet.size();
    LOG(INFO) << "sizeSet = " << sizeSet;
    
    for(int i = 0; i < sizeSet; i++)
    {
        event_del(eventSet[i]);
    }
}

void
handle_timeout(evutil_socket_t fd, short event, void *pArg){

}

void
read_callback(struct bufferevent *pBufEv, void *pArg){
    Manager* pManager = (Manager*)pArg;
    pManager->dispatch(pBufEv);
}

void
write_callback( struct bufferevent * pBufEv, void * pArg ){
    //cout << "write_callback " << endl;
    return ;
}

void
event_callback(struct bufferevent * pBufEv, short sEvent, void * pArg){
    LOG(INFO) << "call event_callback!" ; 
    
    //Manager* pManager = (Manager*)pArg;
    
    //BaseStation* bs_temp = pManager->findBaseStation(pBufEv);
    
    if( BEV_EVENT_CONNECTED == sEvent )
    {
        LOG(INFO) << "BEV_EVENT_CONNECTED == sEvent";
    }

    if( 0 != (sEvent & (BEV_EVENT_ERROR)) )
    {
      int fd = bufferevent_getfd(pBufEv);
      if( fd > 0 )
      {
          evutil_closesocket(fd);
      }
      bufferevent_setfd(pBufEv, -1);
    }
    if (sEvent & BEV_EVENT_EOF) {
        /* connection has been closed, do any clean up here */
        /* ... */
    } else if (sEvent & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    }
    LOG(INFO) << "end call event_callback () !" ;
    
    bufferevent_free(pBufEv);
    return ;
}

void
do_accept(evutil_socket_t listener, short event, void *pArg){
    Manager* temp_manager = (Manager*)pArg;
    struct event_base *base = temp_manager->eventBase();
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) {
        LOG(ERROR) << "error accept fd < 0 ";
        perror("accept");
    }
    else if (fd > FD_SETSIZE) {
        LOG(ERROR) << "fd > FD_SETSIZE ";
        close(fd);
    }
    else {
        LOG(INFO) << "new accept is coming ";
        
        int nRecvBuf=32*1024*1024;
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
 
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        
        BaseStation* temp_bs = new BaseStation(bev, temp_manager);
        temp_manager->addBaseStation(temp_bs);
        temp_bs->setSockaddr((struct sockaddr*)&ss);
                
        bufferevent_setcb(bev, read_callback, write_callback, event_callback, 
                   (void*)temp_manager);
        int config_watermaxRead = temp_manager->config_watermaxRead();
        int config_waterlowRead = temp_manager->config_waterlowRead();
        bufferevent_setwatermark(bev, EV_READ, config_waterlowRead, config_watermaxRead);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }
}

void
run(void){
    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;
    struct event *signal_int;
    
    /* Initalize the event library */
    base = event_base_new();
    if(!base)
        return;

    struct serverConfigureNode* node_options = 
            (struct serverConfigureNode*)malloc(sizeof(struct serverConfigureNode));
     
    node_options->listen_port = 44444;
    node_options->num_baseStation = 2;
    node_options->max_listen_backlog = 16;
    node_options->timer_duration = 200000;
    node_options->water_max_read = 204800;
    node_options->water_low_read = 0;
    node_options->init_num_baseStation = 2;
    const char* configure_path = "../configuration_files/server_configure.json";
    char* pConfigure_file = readfile(configure_path);
	if(pConfigure_file == NULL){
		LOG(ERROR) << "open file " << configure_path << " error.";
		return;
	}
    cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(pConfigure_file);
    free(pConfigure_file);     
    if (!root){
        LOG(ERROR) << " root == NULL ";
    }
    else{
        item = cJSON_GetObjectItem(root, "listen_port");
        node_options->listen_port = item->valueint;
        item = cJSON_GetObjectItem(root, "num_baseStation");
        node_options->num_baseStation = item->valueint;
        item = cJSON_GetObjectItem(root, "max_listen_backlog");
        node_options->max_listen_backlog = item->valueint;
        item = cJSON_GetObjectItem(root, "timer_duration");
        node_options->timer_duration = item->valueint;
        item = cJSON_GetObjectItem(root, "water_max_read");
        node_options->water_max_read = item->valueint;
        item = cJSON_GetObjectItem(root, "water_low_read");
        node_options->water_low_read = item->valueint;
        item = cJSON_GetObjectItem(root, "init_num_baseStation");
        node_options->init_num_baseStation = item->valueint;
        item = cJSON_GetObjectItem(root, "train_mac_addr");
        memcpy(node_options->train_mac_addr,item->valuestring,strlen(item->valuestring)+1);
		item = cJSON_GetObjectItem(root, "local_ip");
        memcpy(node_options->local_ip,item->valuestring,strlen(item->valuestring)+1);
        cJSON_Delete(root);
    }
    
	LOG(INFO) << " local_ip = " << node_options->local_ip ;
    LOG(INFO) << " listen_port = " << node_options->listen_port ;
    LOG(INFO) << " max_listen_backlog = " << node_options->max_listen_backlog ;
    LOG(INFO) << " timer_duration = " << node_options->timer_duration ;
    LOG(INFO) << " water_max_read = " << node_options->water_max_read ;
    LOG(INFO) << " water_low_read = " << node_options->water_low_read ;
    LOG(INFO) << " num_baseStation = " << node_options->num_baseStation ;
    LOG(INFO) << " init_num_baseStation = " << node_options->init_num_baseStation;
    LOG(INFO) << " train_mac_addr = " << node_options->train_mac_addr;
    
    //Singleton design
    Manager* pManager = Manager::getInstance();
    pManager->setBase(base);
    pManager->set_NodeOption(node_options);
    /* Initalize signal event */
    signal_int = evsignal_new(base, SIGINT, signal_callback, (void*)pManager);
    event_add(signal_int, NULL);
    eventSet.push_back(signal_int);
    
    struct event eTimeout;
    struct timeval tTimeout = {0, node_options->timer_duration};
    event_assign(&eTimeout, base, -1, EV_PERSIST, handle_timeout, (void*)pManager);
    evtimer_add(&eTimeout, &tTimeout);
    eventSet.push_back(&eTimeout);
    
// Tcp configure
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(node_options->listen_port);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

#ifndef WIN32
    {
        int one = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }
#endif

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("bind");
        return;
    }

    if (listen(listener, node_options->max_listen_backlog)<0) {
        perror("listen");
        return;
    }
	free(node_options);
    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)pManager);
    /*XXX check it */
    event_add(listener_event, NULL);
    eventSet.push_back(listener_event);    
       
    event_base_dispatch(base);
    
    event_base_free(base);
    
}

int
main(int argc, char **argv)
{ 
    LogAssitant log();
    
    LOG(INFO) << "start main program ";
    
    setvbuf(stdout, NULL, _IONBF, 0);
    
    run();
    
    LOG(INFO) << "exit main program ";
    
    return 0;
}


