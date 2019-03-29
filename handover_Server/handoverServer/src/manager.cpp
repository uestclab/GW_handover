
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <glog/logging.h>

#include "manager.h"
#include "gw_tunnel.h"
#include "signal_json.h"

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

	changeTunnel(linkBs_);

    state_ = glory::RUNNING;
    LOG(INFO) << "state change : RELOCALIZATION ----> RUNNING";
    LOG(INFO) << "uplink and downlink start to work";
}

void Manager::notifyHandover(BaseStation* bs){ 
    nextLinkBs_ = bs;
    BaseStation* next_bs = NULL;
    if(linkBs_ != NULL){
        next_bs = findNextBaseStation(linkBs_);
        if(next_bs != bs){
            LOG(WARNING) << "access bs is not the next neighbour";
        }
    }
	send_start_handover_signal(linkBs_, bs->getBaseStationID(), bs->getBsmac()); // notify linkBs , air send signal to vehicle
    // change tunnel to next link bs
    //...
	LOG(INFO) << "notifyHandover :: START_HANDOVER ";
}

int Manager::incChangeLink(BaseStation* bs, int open){
	if(bs == linkBs_){
		LOG(INFO) << "server receive LINK_CLOSED";	
	}else if(bs == nextLinkBs_){
		LOG(INFO) << "server receive LINK_OPEN";
	}

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
    return pOptions_->water_max_read;
}

int Manager::config_waterlowRead(){
    return pOptions_->water_low_read;
}

void Manager::getTrainMac(char* dest){
	memcpy(dest,pOptions_->train_mac_addr,strlen(pOptions_->train_mac_addr)+1);
}

char* Manager::getLocalIp(){
	char* ip = (char*)malloc(32);
	memcpy(ip,pOptions_->local_ip,strlen(pOptions_->local_ip)+1);
	return ip;
}

// ----------------------------- change tunnel -------------------------------
int Manager::changeTunnel(BaseStation* bs){
	char* remote = bs->getBsIP();
	char* local = getLocalIp();
	change_tunnel(remote,local,NULL);
	delete remote;
	free(local);
}

// ----------------------------- BaseStation Info -------------------------------

int Manager::addBaseStation(BaseStation* bs){
    struct bufferevent* temp = bs->getBufferevent();
    mapBs_.insert(pair<struct bufferevent*,BaseStation*>(temp,bs));
    numBaseStation_ = mapBs_.size();
    LOG(INFO) << "addBaseStation() bs = " << bs << " numBaseStation_ = " << numBaseStation_;
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
			send_init_location_signal(bs_temp, bs_temp->getBaseStationID());// notify bs
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
		send_init_link_signal(activeBs_[index], activeBs_[index]->getBaseStationID(), activeBs_[index]->getBsmac());
        isRelocation = 1;
        LOG(INFO) << "send INIT_LINK signal";
    }
}








