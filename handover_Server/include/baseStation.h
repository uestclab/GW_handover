#ifndef GLORY_BASESTATION_H
#define GLORY_BASESTATION_H

#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event-config.h>
#include <event2/event_struct.h>
#include <event2/util.h>

#include <string>
#include <vector>
#include <sys/types.h>

#include "common.h"
#include "manager.h"
#include "cJSON.h"

#define RECEIVE_BUFFER 1000 * 20
 

using namespace std;


glory::signal_json* clear_json();

class Manager; // forward declaration --- class baseStation call method in class Manager 

class BaseStation
{
public:
    BaseStation(struct bufferevent* bev, Manager* pManager);
    ~BaseStation(void);
    
    // BaseStation Info
    int                    setSockaddr(struct sockaddr* ss);
    struct bufferevent*    getBufferevent();
    int                    getBaseStationID();
    bool                   getIdReady();   
    // receive and process
    int32_t                myNtohl(const char* buf);
    void                   receive();
    void                   processMessage(char* buf, int32_t length);
    glory::messageInfo*    parseMessage(char* buf, int32_t length);
    
    // send meassage
    void                   sendSignal(glory::signalType type, glory::signal_json* json);
    void                   codec(glory::messageInfo* message);
    
    double                 receiverssi();
    void                   mac(char* buf);
        
private:
    Manager*               pManager_;
    // BaseStation infomation
    struct bufferevent*    bev_;
    std::string            ip_;
    int                    baseStationID_;
    bool                   IDReady_;
    //  decodec tcp stream 
    int                    moreData_;
    char                   ReceBuffer_[RECEIVE_BUFFER];
    int                    MinHeaderLen_;
    // 
    double                 receiveRssi_;
    char                   mac_[32];

};

#endif
