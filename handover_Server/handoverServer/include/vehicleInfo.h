#ifndef GLORY_VEHICLE_H
#define GLORY_VEHICLE_H

#include <string>
#include <map>
#include <sys/types.h>

#include "common.h"
#include "manager.h"
#include "baseStation.h"
#include "ThreadPool.h"

using namespace std;

namespace glory
{
/*
    INIT_ACCESS : when receive beacon
    LOCALIZATION : decide init link when init access
    HANDOVER : handover when trigger by other bs
    WORKING : communicate normally
*/
enum veAccessState {INIT_ACCESS=1, LOCALIZATION, HANDOVER, WORKING};

}// end namespcae


class Manager;
class BaseStation;
class VehicleInfo
{
public:
    VehicleInfo(int ve_id, ThreadPool* pThreadpool, Manager* pManager);
    ~VehicleInfo(void);
    
    void recvBeaconAndInit(int bs_id);
    int sendInitDistance();
    void veSaveBsDistance(int bs_id, double dist);
    int makeDecisionFromCandidate();

    void establishLink(int bs_id);
    void incChangeLink(int bs_id, int open);
    void notifyHandover(int bs_id);

    void changeTunnel();

private:
    Manager*                                    pManager_;
    ThreadPool*                                 pThreadpool_;
    enum glory::veAccessState                   state_;

    // ve info 
    int                                         veId_;
    char                                        mac_[32]; // not initialize and not used

    // init access
    map<int, BaseStation*>                      beaconBsCandidate_;
    map<int, double>                            initCandidateDistance_;
    int                                         checkflag; 

    // link info
    int                                         linkBsId_;
    int                                         nextLinkBsId_;
    int                                         changeLinkCnt_; // used for link open and link close

};

#endif // GLORY_VEHICLE_H


    // // move to VehicleInfo
	// int                                       nextbs_expectId_;
