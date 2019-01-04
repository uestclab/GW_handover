#include "manager.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <glog/logging.h>

Manager* Manager::m_pInstance = NULL;
Manager::GarbageCollection Manager::m_gc;


Manager::Manager(){
    LOG(INFO) << "Manager create !";
    base_ = NULL;
    state_ = glory::PAIR_ID;
    countId_ = 0;
    numBaseStation_ = 0;
    maxId_ = -1;
    pOptions_ = (serverConfigureNode*)malloc(sizeof(serverConfigureNode));   
    linkBs_ = NULL;
    nextLinkBs_ = NULL;
    for(int i=0;i<NUM_ACTIVE;i++)
        activeBs_[i] = NULL;
    isRelocation = 0;
    changeLink_ = 0;
}

Manager::~Manager(void){
    for(map<struct bufferevent*,BaseStation*>::iterator it = mapBs_.begin(); 
        it != mapBs_.end(); ++it){
        delete it->second;
    }
    free(pOptions_);
    mapBs_.clear();
    base_ = NULL;
    LOG(INFO) << "~Manager() completed !";
}

void Manager::setBase(struct event_base* base){
    base_ = base;
}

struct event_base* Manager::eventBase(){
    return base_;
}

enum glory::systemState Manager::state(){
    return state_;
}

void Manager::establishLink(BaseStation* bs){
    linkBs_ = bs;
    state_ = glory::RUNNING;
    LOG(INFO) << "state change : RELOCALIZATION ----> RUNNING";
    LOG(INFO) << "uplink and downlink start to work";
}

void Manager::notifyHandover(BaseStation* bs){
    nextLinkBs_ = bs;
    glory::signal_json* json = clear_json();
    json->bsId_ = bs->getBaseStationID();
    bs->mac(json->bsMacAddr_); // important info
    BaseStation* next_bs = NULL;
    if(linkBs_ != NULL){
        next_bs = findNextBaseStation(linkBs_);
        if(next_bs != bs){
            LOG(WARNING) << "access bs is not the next neighbour";
        }
    }
    linkBs_->sendSignal(glory::START_HANDOVER,json); // notify linkBs
    // change tunnel to next link bs
    //...
}

int Manager::incChangeLink(BaseStation* bs, int open){
    changeLink_ = changeLink_ + 1;
    if(changeLink_ == 2){
        changeLink_ = 0;
        linkBs_ = nextLinkBs_;
        return 0;
    }
    return -1;
}

// ----------------------------- configuration file
void Manager::set_NodeOption(serverConfigureNode* options){
    memcpy(pOptions_, options, sizeof(serverConfigureNode));
}

int Manager::config_watermaxRead(){
    LOG(INFO) << "water_max_read " << pOptions_->water_max_read;
    return pOptions_->water_max_read;
}

int Manager::config_waterlowRead(){
    LOG(INFO) << "water_low_read " << pOptions_->water_low_read;
    return pOptions_->water_low_read;
}


// ----------------------------- BaseStation Info -------------------------------

int Manager::addBaseStation(BaseStation* bs){
    struct bufferevent* temp = bs->getBufferevent();
    mapBs_.insert(pair<struct bufferevent*,BaseStation*>(temp,bs));
    numBaseStation_ = mapBs_.size();
    LOG(INFO) << "addBaseStation() bs = " << bs ;
}

BaseStation* Manager::findBaseStation(struct bufferevent *bev){
    map<struct bufferevent*,BaseStation*>::iterator it;
    it = mapBs_.find(bev);
    if(it == mapBs_.end()){
        LOG(ERROR) << "findBaseStation error";
        return NULL;
    }
    else{
        return it->second;
    }    
}

void Manager::completeIdCount(){
    countId_ = countId_ + 1;
    if(countId_ >= pOptions_->num_baseStation){
        CHECK(countId_ == pOptions_->num_baseStation)<<"BaseStation number match access error"; 
        map<struct bufferevent*,BaseStation*>::iterator iter;                
        for(iter = mapBs_.begin() ; iter != mapBs_.end() ; ++iter){
            BaseStation* bs_temp = iter->second;
            glory::signal_json* json = clear_json();
            json->bsId_ = bs_temp->getBaseStationID();
            bs_temp->sendSignal(glory::INIT_LOCATION,json); // notify bs
        }
        state_ = glory::RELOCALIZATION;
        LOG(INFO) << "state change : PAIR_ID ----> RELOCALIZATION";
        countId_ = 0;
    }
}

int Manager::updateIDInfo(BaseStation* bs){
    int id = bs->getBaseStationID();
    if(maxId_ < id)
        maxId_ = id;
    
    map<int,BaseStation*>::iterator it;
    it = relativeLocation_.find(id);
    if(it == relativeLocation_.end()){
        relativeLocation_.insert(pair<int,BaseStation*>(id,bs));
        LOG(INFO) << "new bs_ID = " << id << "  insert in List ";
        return 1;
    }
    else{
        LOG(ERROR) << "duplicate baseStation id insert !";
        return 0;
    }
}

//  encapsulation ---- decodec in each baseStation
void Manager::dispatch(struct bufferevent *bev){
    BaseStation* bs_temp = findBaseStation(bev);
    if(bs_temp != NULL){
        bs_temp->receive();        
    }
}

BaseStation* Manager::findNextBaseStation(BaseStation* bs){
    int id = bs->getBaseStationID();
    if(id == maxId_){
        LOG(WARNING) << "find BaseStation is the last one";
        return bs;
    }
    
    int nextId = id + 1;
    map<int,BaseStation*>::iterator it;
    it = relativeLocation_.find(nextId);
    while(it == relativeLocation_.end()){
        LOG(WARNING) << "baseStation id is not continue ";
        nextId = nextId + 1;
        if(maxId_ < nextId){
            return NULL;
        }
        it = relativeLocation_.find(nextId);
    }
    return it->second;
}
// for INIT_LOCATION and RELOCALIZATION
void Manager::init_num_check(BaseStation* bs){
    activeBs_[countId_] = bs;
    countId_ = countId_ + 1;
    if(countId_ >= pOptions_->init_num_baseStation && isRelocation == 0){
        CHECK(countId_ == pOptions_->init_num_baseStation)<<"init number error";
        int index = -1;
        double max = -99;
        for(int i=0;i<countId_;i++){
            if(activeBs_[i]->receiverssi() > max){
                max = activeBs_[i]->receiverssi();
                index = i;
            }
        }
        glory::signal_json* json = clear_json();
        json->bsId_ = activeBs_[index]->getBaseStationID();
        activeBs_[index]->mac(json->bsMacAddr_);
        activeBs_[index]->sendSignal(glory::INIT_LINK,json);
        isRelocation = 1;
        LOG(INFO) << "send INIT_LINK signal";
    }
}








