#include "vehicleInfo.h"
#include <cstring>
#include <string>
#include "ThreadFuncWrapper.h"
#include "signal_json.h"
#include <glog/logging.h>

using namespace std;

// ----------------------------------- initialize class --------------------------------------

VehicleInfo::VehicleInfo(int ve_id, ThreadPool* pThreadpool, Manager* pManager){
    pManager_ = pManager;
    pThreadpool_ = pThreadpool;
    state_ = glory::INIT_ACCESS;
    linkBsId_ = -1;
    nextLinkBsId_ = -1;
    changeLinkCnt_ = 0;
    veId_ = ve_id;
    checkflag = 0;
    beaconBsCandidate_.clear();
    initCandidateDistance_.clear();
}

VehicleInfo::~VehicleInfo(void){
    if(pManager_)
        pManager_ = NULL;
}


void VehicleInfo::recvBeaconAndInit(int bs_id){
    BaseStation* bs = pManager_->findBsById(bs_id);
    if(bs == NULL){
        return;
    }
    map<int,BaseStation*>::iterator it;
    it = beaconBsCandidate_.find(bs_id);
    if(it == beaconBsCandidate_.end() && checkflag == 0){
        if(beaconBsCandidate_.size() == 0){
            // start timer
            postTimeOutWorkToThreadPool(veId_, pManager_, pThreadpool_);
        } 
        beaconBsCandidate_.insert(pair<int, BaseStation*>(bs_id,bs)); 
        LOG(INFO) << "insert id : " << bs_id;    
    }
}

int VehicleInfo::sendInitDistance(){
    if(checkflag == 0){
        checkflag = 1;
    }
    if(beaconBsCandidate_.size() > 0){
        map<int,BaseStation*>::iterator it = beaconBsCandidate_.begin();
        BaseStation* bs = it->second;
        send_init_distance_signal(bs, bs->getBaseStationID(), veId_);// notify bs to init measure distance
        beaconBsCandidate_.erase(bs->getBaseStationID());
        return 1;
    }else{
        return 0;
    }
}

void VehicleInfo::veSaveBsDistance(int bs_id, double dist){
    initCandidateDistance_.insert(pair<int,double>(bs_id,dist));
}

int VehicleInfo::makeDecisionFromCandidate(){
    map<int,double>::iterator iter;
    int maxBs_id = -1;
    double maxDist = -1;                
    for(iter = initCandidateDistance_.begin() ; iter != initCandidateDistance_.end() ; ++iter){
        if(maxDist < iter->second){
            maxDist = iter->second;
            maxBs_id = iter->first;
        }
    }
    return maxBs_id;
}

void VehicleInfo::establishLink(int bs_id){
    linkBsId_ = bs_id;
    BaseStation* linkBs = pManager_->findBsById(linkBsId_);
    pManager_->changeTunnel(linkBs);
}

void VehicleInfo::incChangeLink(int bs_id, int open){
	if(bs_id == linkBsId_ && open == 0){
		LOG(INFO) << "server receive LINK_CLOSED bs id " << bs_id;
    }else if(bs_id == nextLinkBsId_ && open == 1){
		LOG(INFO) << "server receive LINK_OPEN bs id " << bs_id;
	}

    changeLinkCnt_ = changeLinkCnt_ + 1;
    if(changeLinkCnt_ == 2){
        changeLinkCnt_ = 0;
        //linkBs_ = nextLinkBs_;
        //nextLinkBs_ = NULL;
        linkBsId_ = nextLinkBsId_;
        nextLinkBsId_ = -1;
    }
}

void VehicleInfo::notifyHandover(int bs_id){
    nextLinkBsId_ = bs_id;
    BaseStation* linkBs = pManager_->findBsById(linkBsId_);
    BaseStation* readyBs = pManager_->findBsById(nextLinkBsId_);
    send_start_handover_signal(linkBs, readyBs, readyBs->getBaseStationID(), readyBs->getBsmac()); // notify linkBs , air send signal to vehicle
}

void VehicleInfo::changeTunnel(){
    BaseStation* nextLinkBs = pManager_->findBsById(nextLinkBsId_);
    pManager_->changeTunnel(nextLinkBs);
    LOG(INFO) << "START_HANDOVER --> tunnel change to target bs and buffer in , nextLinkBs = " << nextLinkBs->getBaseStationID();
}