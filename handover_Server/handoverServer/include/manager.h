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
#include <map>
#include <set>
#include <string>
#include <stdlib.h>
#include <glog/logging.h>
#define NUM_ACTIVE 6
using namespace std;
struct NodeOptions;
class BaseStation;
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
    void                               completeIdCount();
    int                                updateIDInfo(BaseStation* bs);
    // receive and dispatch message
    void                               dispatch(struct bufferevent *bev);
    void                               init_num_check(BaseStation* bs);
    //stateMachine
    enum glory::systemState            state();
    void                               establishLink(BaseStation* bs);
    void                               notifyHandover(BaseStation* ready_bs);
    int                                incChangeLink(BaseStation* bs, int open);
    //configuration file
    void                               set_NodeOption(serverConfigureNode* options);
    int                                config_watermaxRead();
    int                                config_waterlowRead();
    int                                config_num_baseStation();
	char*                              getLocalIp();

	//change tunnel
	int                                changeTunnel(BaseStation* bs);
	void                               change_tunnel_Link(); // in running state , tunnel change to target bs

	// sequence handover
	int                                next_expectId_check(BaseStation* bs);
	void                               updateExpectId(BaseStation* bs);
	void                               recall_other_bs(BaseStation* bs);
   void                                resetManager(void);

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

    map<int,BaseStation*>                     relativeLocation_;// <key,mapped_type>
    int                                       maxId_;
    int                                numBaseStation_;

    int                                countId_; // 3 diffrent used for counter 
    serverConfigureNode*               pOptions_;
    int                                reset_count_;
    
    enum glory::systemState            state_;
    BaseStation*                       linkBs_;
    BaseStation*                       nextLinkBs_;
    BaseStation*                       activeBs_[NUM_ACTIVE];
    int                                isRelocation;
    int                                changeLink_;
	int                                nextbs_expectId_;
};

#endif




