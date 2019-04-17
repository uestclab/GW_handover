#include "baseStation.h"
#include <cstring>
#include <string>

#include <glog/logging.h>
#include "gw_tunnel.h"
#include "signal_json.h"

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
            LOG(INFO) << "signal : " << item->valuestring;
            if(getIdReady() == false){
                item = cJSON_GetObjectItem(root, "bs_id");
                baseStationID_ = item->valueint;
                item = cJSON_GetObjectItem(root, "bs_mac_addr");
                //mac_
                memcpy(mac_,item->valuestring,strlen(item->valuestring)+1);
				LOG(INFO) << "mac : " << mac_;
                IDReady_ = true;
		        send_id_received_signal(this,baseStationID_);
                pManager_->updateIDInfo(this);
                pManager_->completeIdCount();
            }else{
				send_id_received_signal(this,baseStationID_);
			}
            break;
        }
        case glory::READY_HANDOVER:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << item->valuestring;
            if(pManager_->state() == glory::RELOCALIZATION){
                item = cJSON_GetObjectItem(root, "quility");
                pManager_->init_num_check(this);
            }else if(pManager_->state() == glory::RUNNING){
                item = cJSON_GetObjectItem(root, "bs_id");
				LOG(INFO) << "bs_id : " << item->valuedouble;
                pManager_->notifyHandover(this);
            }
            break;
        }
        case glory::INIT_COMPLETED: // init_num_check 
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << item->valuestring;
            // tunnel set in establishLink() -----------
            pManager_->establishLink(this);
            break;
        }
        case glory::LINK_CLOSED:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << item->valuestring;
            pManager_->incChangeLink(this,0);
            break;
        }
        case glory::LINK_OPEN:
        {
            item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << item->valuestring;
            pManager_->incChangeLink(this,1);
            break;
        }
		case glory::CHANGE_TUNNEL:
		{
			item = cJSON_GetObjectItem(root, "signal");
            LOG(INFO) << "signal : " << item->valuestring;
			pManager_->change_tunnel_Link();
			send_change_tunnel_ack_signal(this, baseStationID_);
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
    LOG(INFO) << "BaseStation ip = " << ip_ ;
}

char* BaseStation::getBsIP(){
	char* cstr = new char [ip_.length()+1];
  	std::strcpy (cstr, ip_.c_str());
	return cstr; 
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

char* BaseStation::getBsmac(){
	char* cstr = mac_;
	return cstr;
}
// --------------------------------------- send meassage ------------------- 

void BaseStation::sendSignal(glory::signalType type, char* json){
    glory::messageInfo* message = new glory::messageInfo();
    message->signal = type;
    message->buf = json;
	message->length = strlen(message->buf) + 1;
	LOG(INFO) << endl << " server send : cjson = " << message->buf;
    codec(message);
	free(json);
    delete message;
}

void BaseStation::codec(glory::messageInfo* message){
    std::string result_temp;
    result_temp.resize(MinHeaderLen_); // reserve for messageLength;  
    int32_t nameLen = static_cast<int32_t>(message->signal);
    int32_t be32 = htonl(nameLen);
    result_temp.append(reinterpret_cast<char*>(&be32), sizeof(be32));
    if (message->buf)
    {
        result_temp.append(message->buf,strlen(message->buf) + 1);        
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




