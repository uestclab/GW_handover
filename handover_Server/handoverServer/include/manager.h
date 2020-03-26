#ifndef GLORY_MANAGER_H
#define GLORY_MANAGER_H

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event-config.h>
#include <event2/event_struct.h>
#include <event2/util.h>

#include "common.h"
#include "baseStation.h"
#include "vehicleInfo.h"
#include "EventHandlerQueue.h"
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <glog/logging.h>

using namespace std;
struct NodeOptions;
class BaseStation;
class VehicleInfo;

class Manager{
public:
    static Manager*                    getInstance(){
        if (m_pInstance == NULL)
            m_pInstance = new Manager();  
        return m_pInstance;
    } 
    void                               setBase(struct event_base* base);
    struct event_base*                 eventBase();
    //Manage BaseStation Info
    int                                addBaseStation(BaseStation* bs);
    BaseStation*                       findBaseStation(struct bufferevent *bev);
    BaseStation*                       findNextBaseStation(BaseStation* bs);
    void                               tryCompleteBsAccess(); // inform all access bs monitor air interface
    int                                updateIDInfo(BaseStation* bs);
    BaseStation*                       findBsById(int id);
    // receive and dispatch message
    void                               dispatch(struct bufferevent *bev);
    //stateMachine
    enum glory::systemState            state();
    void                               establishLink(BaseStation* bs, int ve_id);
    void                               notifyHandover(BaseStation* ready_bs);
    int                                incChangeLink(int bs_id, int ve_id, int open);
    //configuration file
    void                               set_NodeOption(serverConfigureNode* options);
    int                                config_watermaxRead();
    int                                config_waterlowRead();
    int                                config_num_baseStation();
	char*                              getLocalIp();

	//change tunnel
	int                                changeTunnel(BaseStation* bs);
	void                               change_tunnel_Link(int ve_id); // in running state , tunnel change to target bs

	// sequence handover ????????????????? ---- need change , not by id?
	int                                next_expectId_check(BaseStation* bs);
	void                               updateExpectId(BaseStation* bs);
	void                               recall_other_bs(BaseStation* bs);
    void                               resetManager(void);

    // Manager Ve init access
    void                               transferToVe(int ve_id, int bs_id);
    VehicleInfo*                       findVeById(int ve_id);
    void                               informInitDistance(int ve_id);
    void                               saveBsDistance(int ve_id, int bs_id, double dist);

    //event process handler
    void                               StartEventProcess();
    int                                post_msg(long int msg_type, char *buf, int buf_len, int bs_id, int ve_id, double dist=0);

private:    
    Manager();
    ~Manager(void);
    static Manager* m_pInstance;
    class GarbageCollection // 垃圾回收类 
    {
    public:
        GarbageCollection()  
        {  
            LOG(INFO) << "GarbageCollection construction";  
        }  
        ~GarbageCollection()
        {  
            if (Manager::m_pInstance)
                delete Manager::m_pInstance;
        }
    };
    
    //把复制构造函数和=操作符设为私有,防止被复制
    Manager(const Manager&){};
    Manager& operator=(const Manager&){};
    static GarbageCollection m_gc;  //垃圾回收类的静态成员
    
    // ----------------------------------------------------------------
    //For BaseStation
    map<struct bufferevent*,BaseStation*>     mapBs_;    
    event_base*                               base_; // only transfer base variable to libevent , no use   

    map<int,BaseStation*>                     mapBsId_;// <key,mapped_type>
    int                                       maxId_;
    int                                       numBaseStation_;

    // VehicleInfo
    map<int, VehicleInfo*>                    mapVe_;

    int                                       countId_; // 3 diffrent used for counter 
    serverConfigureNode*                      pOptions_;
    int                                       reset_count_;
    
    enum glory::systemState                   state_;
    // move to VehicleInfo
	int                                       nextbs_expectId_;

    /* thread pool handler */
    ThreadPool*                               pThreadpool_;
    /* event process handler , include queue*/
    EventHandlerQueue*                        pEventHandlerQueue_;
};

#endif




