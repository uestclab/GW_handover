#include "baseStation.h"
#include <cstring>
#include <string>

#include <glog/logging.h>

using namespace std;

// ----------------------------------- initialize class --------------------------------------

BaseStation::BaseStation(struct bufferevent* bev , Manager* pManager){
    pManager_ = pManager;
    bev_ = bev;
    baseStationID_ = -1;
    IDReady_ = false;
    ip_ = "0.0.0.0";
    moreData_ = 0;
    MinHeaderLen_ = sizeof(int32_t);
    receiveRssi_ = -99;
}


BaseStation::~BaseStation(void){
    if(bev_){
        LOG(INFO) << "BaseStation ID = " << baseStationID_;
        LOG(INFO) << "~BaseStation() "; 
    }
    if(pManager_)
        pManager_ = NULL;
}

// ------------------------------------------------------------------------------------

int32_t BaseStation::myNtohl(const char* buf){
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return ::ntohl(be32);
}

void BaseStation::receive(){  
    size_t n;
    int size = 0;
    int totalByte = 0;
    int messageLength = 0;
    
    char* temp_receBuffer = ReceBuffer_ + 400; // hard code for offset = 100 
    char* pStart = NULL;
    char* pCopy = NULL;
    while(true){
        n = bufferevent_read(bev_, temp_receBuffer, RECEIVE_BUFFER);
        if(n<0){
            LOG(ERROR) << "error bufferevent_read size n < 0 !!!!!!!!!!";
        }
        if(n <= 0)
            break;
        size = size + n;
    }
    
    
    pStart = temp_receBuffer - moreData_;
    totalByte = size + moreData_;
    
    const int MinHeaderLen = sizeof(int32_t); 
    
    while(1){
        if(totalByte <= MinHeaderLen)
        {
            moreData_ = totalByte;
            pCopy = pStart;
            if(moreData_ > 0)
            {
                memcpy(temp_receBuffer - moreData_, pCopy, moreData_);
            }
            break;
        }
        if(totalByte > MinHeaderLen)
        {
            messageLength= myNtohl(pStart);

            if(totalByte < (messageLength + MinHeaderLen) )
            {
                moreData_ = totalByte;
                pCopy = pStart;
                if(moreData_ > 0){
                    memcpy(temp_receBuffer - moreData_, pCopy, moreData_);
                }
                break;
            } 
            else 
            {
                processMessage(pStart,messageLength);
                pStart = pStart + messageLength + MinHeaderLen;
                totalByte = totalByte - messageLength - MinHeaderLen;
                if(totalByte == 0){
                    moreData_ = 0;
                    break;
                }
            }//else             
        }//(totalByte > MinHeaderLen)
    } //while(1)
}

glory::messageInfo* BaseStation::parseMessage(char* buf, int32_t length){
    glory::messageInfo* message = new glory::messageInfo();
    message->length = length;
    message->signal = (glory::signalType)(myNtohl(buf+sizeof(int32_t)));
    message->buf = buf+sizeof(int32_t)*2;
    return message;
}

void BaseStation::processMessage(char* buf, int32_t length){
    glory::messageInfo* message = parseMessage(buf,length);
    cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(message->buf);
    switch(message->signal){
        case glory::ID_PAIR:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << cJSON_Print(item);
            if(getIdReady() == false){
                item = cJSON_GetObjectItem(root, "bs_id");
                baseStationID_ = item->valueint;
                item = cJSON_GetObjectItem(root, "bs_mac_addr");
                //mac_
                memcpy(mac_,item->string,strlen(item->string)+1);
                IDReady_ = true;
                pManager_->updateIDInfo(this);
                pManager_->completeIdCount();
            }
            glory::signal_json* json = clear_json();
            json->bsId_ = baseStationID_;
            sendSignal(glory::ID_RECEIVED,json);
            break;
        }
        case glory::READY_HANDOVER:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << cJSON_Print(item);
            if(pManager_->state() == glory::RELOCALIZATION){
                item = cJSON_GetObjectItem(root, "rssi");
                receiveRssi_ = item->valuedouble;
                pManager_->init_num_check(this);
            }else if(pManager_->state() == glory::RUNNING){
                item = cJSON_GetObjectItem(root, "rssi");
                receiveRssi_ = item->valuedouble;
                pManager_->notifyHandover(this);
            }
            break;
        }
        case glory::INIT_COMPLETED:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << cJSON_Print(item);
            // tunnel set ----------- to do
            pManager_->establishLink(this);
            break;
        }
        case glory::LINK_CLOSED:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << cJSON_Print(item);
            pManager_->incChangeLink(this,0);
            break;
        }
        case glory::LINK_OPEN:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << cJSON_Print(item);
            pManager_->incChangeLink(this,1);
            break;
        }
        default:
        {
            break;
        }
    }
    delete message;
}

//-------------------------------BaseStation Info -------------------------------------------------------
struct bufferevent* BaseStation::getBufferevent(){
    return bev_;
}
int BaseStation::setSockaddr(struct sockaddr* ss){
    sockaddr_in* pSin = (sockaddr_in*)ss;
    ip_ = inet_ntoa(pSin->sin_addr);
    LOG(INFO) << "ip = " << ip_ ;
}
int BaseStation::getBaseStationID(){
    return baseStationID_;
}
bool BaseStation::getIdReady(){
    return IDReady_;
}
double BaseStation::receiverssi(){
    return receiveRssi_;
}
void BaseStation::mac(char* buf){
    memcpy(buf,mac_,32);
}
// --------------------------------------- send meassage ------------------- 
glory::signal_json* clear_json(){
    glory::signal_json* input = (glory::signal_json*)malloc(sizeof(glory::signal_json));
    input->bsId_ = -1;
    input->rssi_ = -99;
    memset(input->bsMacAddr_,0x00,32);
    memset(input->trainMacAddr_,0x00,32);
    return input;
}

void BaseStation::sendSignal(glory::signalType type, glory::signal_json* json){
    glory::messageInfo* message = new glory::messageInfo();
    message->length = 0;
    message->signal = type;
    cJSON* root = cJSON_CreateObject();
    if(type == glory::ID_RECEIVED)
        cJSON_AddItemToObject(root, "signal", cJSON_CreateString("id_received_signal"));
    else if(type == glory::INIT_LOCATION)
        cJSON_AddItemToObject(root, "signal", cJSON_CreateString("init_location_signal"));
    else if(type == glory::INIT_LINK)
        cJSON_AddItemToObject(root, "signal", cJSON_CreateString("init_link_signal"));
    else if(type == glory::START_HANDOVER)
        cJSON_AddItemToObject(root, "signal", cJSON_CreateString("start_handover_signal"));
    cJSON_AddItemToObject(root, "bs_id", cJSON_CreateNumber(json->bsId_));
    cJSON_AddItemToObject(root, "rssi", cJSON_CreateNumber(json->rssi_));
    cJSON_AddStringToObject(root, "bs_mac_addr", json->bsMacAddr_);
    cJSON_AddStringToObject(root, "train_mac_addr", json->trainMacAddr_);
    message->buf = cJSON_Print(root);
    codec(message);
    free(message->buf);
    delete message;
    free(json);
    cJSON_Delete(root);
}

void BaseStation::codec(glory::messageInfo* message){
    std::string result_temp;
    result_temp.resize(MinHeaderLen_); // reserve for messageLength;  
    int32_t nameLen = static_cast<int32_t>(message->signal);
    int32_t be32 = htonl(nameLen);
    result_temp.append(reinterpret_cast<char*>(&be32), sizeof(be32));
    if (message->buf)
    {
        result_temp.append(message->buf,strlen(message->buf));        
        int32_t messageLength = ::htonl(result_temp.size() - MinHeaderLen_);
        std::copy(reinterpret_cast<char*>(&messageLength),
                  reinterpret_cast<char*>(&messageLength) + sizeof(messageLength),
                  result_temp.begin());
    }
    else
    {
        LOG(ERROR) << "No cjson payload";
    }   
    bufferevent_write(bev_, result_temp.c_str(), result_temp.size());
    LOG(INFO) << "codec successful";
}




