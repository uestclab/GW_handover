
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <glog/logging.h>

#include "manager.h"
#include "gw_tunnel.h"
#include "signal_json.h"
#include "ThreadFuncWrapper.h"

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
    nextbs_expectId_ = 0;
    reset_count_ = 0;
    pEventHandlerQueue_ = NULL;
    pThreadpool_ = NULL;
    createThreadPool(128, 8, &pThreadpool_);

    mapVe_.clear();
}

// for test reset system when restart bs on board
void Manager::resetManager(void){
    reset_count_ = reset_count_ + 1;
    if(reset_count_ != config_num_baseStation()){
	LOG(INFO) << "reset_count_ = " << reset_count_;
        return;
    }
    state_ = glory::PAIR_ID;
    countId_ = 0;
    numBaseStation_ = 0;
    maxId_ = -1;
    nextbs_expectId_ = 0;
    reset_count_ = 0;
    mapBs_.clear();
    mapBsId_.clear();
    LOG(INFO) << "resetManager !";
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

void Manager::change_tunnel_Link(int ve_id){

    VehicleInfo* temp_ve = findVeById(ve_id);
    if(temp_ve != NULL){
        temp_ve->changeTunnel();
    }

	//changeTunnel(nextLinkBs_);
	// LOG(INFO) << "state hold : RUNNING ----> RUNNING";
    // LOG(INFO) << "START_HANDOVER --> tunnel change to target bs and buffer in , nextLinkBs_ = " << nextLinkBs_->getBaseStationID();
}

void Manager::establishLink(BaseStation* bs, int ve_id){

	//changeTunnel(bs);

    VehicleInfo* temp_ve = findVeById(ve_id);
    if(temp_ve != NULL){
        temp_ve->establishLink(bs->getBaseStationID());
    }

    state_ = glory::RUNNING;
    //LOG(INFO) << "state change : RELOCALIZATION ----> RUNNING";
    LOG(INFO) << "uplink and downlink start to work, link bs id = " << bs->getBaseStationID();
}

void Manager::notifyHandover(BaseStation* ready_bs){
    // nextLinkBs_ = ready_bs;
    // BaseStation* next_bs = NULL;
    // if(linkBs_ != NULL){
    //     next_bs = findNextBaseStation(linkBs_);
    //     if(next_bs != ready_bs){
    //         LOG(WARNING) << "access ready_bs is not the next neighbour";
    //     }
    // }
	// send_start_handover_signal(linkBs_, ready_bs, ready_bs->getBaseStationID(), ready_bs->getBsmac()); // notify linkBs , air send signal to vehicle

	// LOG(INFO) << "notifyHandover :: START_HANDOVER --- tunnel wait change_tunnel signal "; //


    /* ready handover bs do not know ve_id , can't transfer to manager??? --- 0326 */
    // VehicleInfo* temp_ve = findVeById(ve_id);
    // if(temp_ve != NULL){
    //     temp_ve->notifyHandover(ready_bs->getBaseStationID());
    // }
}

int Manager::incChangeLink(int bs_id, int ve_id, int open){
    VehicleInfo* temp_ve = findVeById(ve_id);
    if(temp_ve != NULL){
        temp_ve->incChangeLink(bs_id, open);
    }
}

// ----------------------------- configuration file
void Manager::set_NodeOption(serverConfigureNode* options){
    memcpy(pOptions_, options, sizeof(serverConfigureNode));
}

int Manager::config_num_baseStation(){
    return pOptions_->num_baseStation;
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
	//change_tunnel(remote,local,NULL);
	delete remote;
	free(local);
}

// ----------------------------- BaseStation Info -------------------------------

int Manager::addBaseStation(BaseStation* bs){
    struct bufferevent* temp = bs->getBufferevent();
    mapBs_.insert(pair<struct bufferevent*,BaseStation*>(temp,bs));
    numBaseStation_ = mapBs_.size();
    //LOG(INFO) << "addBaseStation() bs = " << bs << " numBaseStation_ = " << numBaseStation_;
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

void Manager::tryCompleteBsAccess(){
    countId_ = countId_ + 1;
    if(countId_ >= pOptions_->num_baseStation){
        CHECK(countId_ == pOptions_->num_baseStation)<<"BaseStation number match access error"; 
        map<struct bufferevent*,BaseStation*>::iterator iter;                
        for(iter = mapBs_.begin() ; iter != mapBs_.end() ; ++iter){
            BaseStation* bs_temp = iter->second;
			send_init_location_signal(bs_temp, bs_temp->getBaseStationID());// notify bs
        }
        state_ = glory::RUNNING;
        LOG(INFO) << "state change : PAIR_ID ----> RUNNING";
        countId_ = 0;
    }
}

int Manager::updateIDInfo(BaseStation* bs){
    int id = bs->getBaseStationID();
    if(maxId_ < id)
        maxId_ = id;
    
    map<int,BaseStation*>::iterator it;
    it = mapBsId_.find(id);
    if(it == mapBsId_.end()){
        mapBsId_.insert(pair<int,BaseStation*>(id,bs));
        LOG(INFO) << "new bs_ID = " << id << "  insert in List ";
        return 1;
    }
    else{
        LOG(ERROR) << "duplicate baseStation id insert !";
        return 0;
    }
}

BaseStation* Manager::findBsById(int id){
    map<int,BaseStation*>::iterator it;
    it = mapBsId_.find(id);
    if(it == mapBsId_.end()){
        return NULL;
    }else{
        return it->second;
    }  
}

VehicleInfo* Manager::findVeById(int id){
    map<int,VehicleInfo*>::iterator it;
    it = mapVe_.find(id);
    if(it == mapVe_.end()){
        return NULL;
    }else{
        return it->second;
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
    it = mapBsId_.find(nextId);
    while(it == mapBsId_.end()){
        LOG(WARNING) << "baseStation id is not continue ";
        nextId = nextId + 1;
        if(maxId_ < nextId){
            return NULL;
        }
        it = mapBsId_.find(nextId);
    }
    return it->second;
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
	if(nextbs_expectId_ == ( pOptions_->num_baseStation * 11 + 11)){
		nextbs_expectId_ = 11;
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

/* new add */
void Manager::StartEventProcess(){
    pEventHandlerQueue_ = new EventHandlerQueue(this);
    pEventHandlerQueue_->start_event_process_thread(pThreadpool_);
}

int Manager::post_msg(long int msg_type, char *buf, int buf_len, int bs_id, int ve_id, double dist){
    pEventHandlerQueue_->post_msg(msg_type,buf,buf_len, bs_id, ve_id, dist);
}

// find ve , and transfer beacon info to ve
void Manager::transferToVe(int ve_id, int bs_id){
    map<int,VehicleInfo*>::iterator it;
    VehicleInfo* temp_ve = NULL;
    it = mapVe_.find(ve_id);
    if(it == mapVe_.end()){
        // new and insert
        temp_ve = new VehicleInfo(ve_id, pThreadpool_, this);
        mapVe_.insert(pair<int,VehicleInfo*>(ve_id,temp_ve));
        LOG(INFO) << "mapVe insert -- ve_id : " << ve_id << "bs_id : " << bs_id;
    }else{
        temp_ve = it->second;
        LOG(INFO) << "mapVe already have -- ve_id : " << it->first << "bs_id : " << bs_id;
    }

    temp_ve->recvBeaconAndInit(bs_id);
}

void Manager::informInitDistance(int ve_id){
    VehicleInfo* temp_ve = findVeById(ve_id);
    int ret = temp_ve->sendInitDistance();
    if(ret == 0){
        // no more candidate, start make decision and no event come
        int candidate_bsId = temp_ve->makeDecisionFromCandidate();
        // trigger candidate bs as init link bs
        BaseStation* candidate_bs = findBsById(candidate_bsId);
        send_init_link_signal(candidate_bs, candidate_bs->getBaseStationID(), candidate_bs->getBsmac()); // end of init access by ve -- lq 20200326
    }
}

void Manager::saveBsDistance(int ve_id, int bs_id, double dist){
    VehicleInfo* temp_ve = findVeById(ve_id);
    if(temp_ve != NULL){
        temp_ve->veSaveBsDistance(bs_id, dist);
    }
}






