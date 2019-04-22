
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
	nextbs_expectId_ = 0;
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

void Manager::change_tunnel_Link(){
	changeTunnel(nextLinkBs_);
	LOG(INFO) << "state hold : RUNNING ----> RUNNING";
    LOG(INFO) << "START_HANDOVER --> tunnel change to target bs and buffer in , nextLinkBs_ = " << nextLinkBs_->getBaseStationID();
}

void Manager::establishLink(BaseStation* bs){
    linkBs_ = bs;

	changeTunnel(linkBs_);

    state_ = glory::RUNNING;
    LOG(INFO) << "state change : RELOCALIZATION ----> RUNNING";
    LOG(INFO) << "uplink and downlink start to work, link bs id = " << linkBs_->getBaseStationID();
}

void Manager::notifyHandover(BaseStation* ready_bs){ 
    nextLinkBs_ = ready_bs;
    BaseStation* next_bs = NULL;
    if(linkBs_ != NULL){
        next_bs = findNextBaseStation(linkBs_);
        if(next_bs != ready_bs){
            LOG(WARNING) << "access ready_bs is not the next neighbour";
        }
    }
	send_start_handover_signal(linkBs_, ready_bs->getBaseStationID(), ready_bs->getBsmac()); // notify linkBs , air send signal to vehicle

	LOG(INFO) << "notifyHandover :: START_HANDOVER --- tunnel wait change_tunnel signal "; // 
}

int Manager::incChangeLink(BaseStation* bs, int open){
	if(bs == linkBs_){
		LOG(INFO) << "server receive LINK_CLOSED bs id " << bs->getBaseStationID();	
	}else if(bs == nextLinkBs_){
		LOG(INFO) << "server receive LINK_OPEN bs id " << bs->getBaseStationID();
	}

    changeLink_ = changeLink_ + 1;
    if(changeLink_ == 2){
        changeLink_ = 0;
        linkBs_ = nextLinkBs_;
		nextLinkBs_ = NULL;
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
    
    int nextId = id + 11;
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
		int index = 0;
		if(bs->getBaseStationID() == 22){	
			send_init_link_signal(activeBs_[index], activeBs_[index]->getBaseStationID(), activeBs_[index]->getBsmac());
		    isRelocation = 1;
		    LOG(INFO) << "send INIT_LINK signal";
		}else{
			LOG(INFO) << " INIT_LINK ready_handover is not bs_id == 22";
		}
		countId_ = 0;
    }
}

//sequence handover
int Manager::next_expectId_check(BaseStation* bs){
	if(nextbs_expectId_ == bs->getBaseStationID())
		return 0;
	else
		return -1;
}

void Manager::updateExpectId(BaseStation* bs){
	nextbs_expectId_ = bs->getBaseStationID() + 11;
	if(nextbs_expectId_ == ( pOptions_->num_baseStation * 11 + 22)){
		nextbs_expectId_ = 22;
	}
}

void Manager::recall_other_bs(BaseStation* bs){
	map<struct bufferevent*,BaseStation*>::iterator iter;                
	for(iter = mapBs_.begin() ; iter != mapBs_.end() ; ++iter){
		BaseStation* bs_temp = iter->second;
		if(bs_temp == bs)
			continue;
		send_server_recall_monitor_signal(bs_temp, bs_temp->getBaseStationID());// notify all other bs to monitor
	}
}







