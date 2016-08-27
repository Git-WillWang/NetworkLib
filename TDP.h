#pragma once
#ifndef TDP_H
#define TDP_H
#include "NetworkLibType.h"
#include "InternalPackageManager.h"
#include "BitStream.h"
#include <list>
#include <map>
#include <time.h>
#include <queue>
#include <vector>
#include <sys/socket.h>
#include <utility>
using namespace std;
#define UDP_HEADER_SIZE 28 //UDP包头大小
#ifndef DEFAULT_MTU_SIZE
#define DEFAULT_MTU_SIZE 576 //默认数据长度
#define MAXIMUM_MTU_SIZE 8000 //最大数据长度
#define MAX_ORDER_SIZE 255
#endif // !DEFAULT_MTU_SIZE
class TDP {
public:
    TDP(const unsigned int timeout,const unsigned int maxPackagePerSec);
    TDP();~TDP();
    void initLog(int timeout, int maxPackagePerSec);
    InternalPackage* createInternalPackage(BitStream*);
    void handleRecData(unsigned char* data,int length);
    void sendConfirmPackage(unsigned int id);
    void free();
    InternalPackage* buildPackageFromSplitPackageMap(unsigned int id);
    void mainLoop(SOCKET s, ConnID id, int MTUSize);
    bool isResendQueueOverflow();
    bool needToSend();
    void generateFrame(BitStream* output, int MTUSizeb, bool& isReliablePackageSent);
    void writeToBitStream(BitStream* b, const InternalPackage* const p);
    void splitPackage(InternalPackage* p);
    bool send(BitStream* b, PackagePriority priority, PackageReliability reliability, unsigned char streamIndex);
    void sendBitStream(SOCKET s,ConnID connId,BitStream* b);
    void setMTU(const unsigned int size);
    unsigned int getMTU();
    void setLostPackageResendDelay(unsigned int i);
private:
    class ResendQueue{
    public:
        ResendQueue(){
            Queue = new map<pair<unsigned short,time_t>,InternalPackage*,keyCompare>();
        }
        void push(InternalPackage* p){
            if(!p) return;
            pair<unsigned short,time_t> key(p->id,p->nextActTime);
            Queue->insert(make_pair(key,p));
        }
        InternalPackage* pop(){
            if(!Queue.empty()){
                pair<unsigned short,time_t> key = Queue->begin().first;
                InternalPackage* res = Queue->begin().second;
                Queue->erase(key);
                return res;
            }
        }
        void erase(InternalPackage* p){
            pair<unsigned short,time_t> key(p->id,p->nextActTime);
            Queue->erase(key);
        }
        bool empty(){
            return Queue->empty();
        }
        int size(){
            return Queue->size();
        }
        InternalPackage* front(){
            return (Queue->begin();
        }
        void clear(){
            map<pair<unsigned short,time_t>,InternalPackage*,keyCompare>::iterator i;
            for(i = Queue->begin();i!=Queue->end();){
                p = i.value;
                internalPackageManager->release(p,true);
                Queue->erase(i++);
            }
        }
    private:
        struct keyCompare{
            bool operator()(const pair<unsigned short,time_t>& key1,const pair<unsigned short,time_t>& key2){
                return key1.second < key2.second;
            }
        };
        map<pair<unsigned short,time_t>,InternalPackage*,keyCompare>* Queue;
    };
    unsigned int numOfAskConfirm =0;
    unsigned int numOfRecvPackage = 0;
    unsigned int numOfOrderedStream = 0;
    unsigned int windowSize =0;
    unsigned int lossyWindowSize =0;
    unsigned int numOfPackageSent = 0;
    unsigned int bitStreamSent =0;
    unsigned int resendPackageSent = 0;
    unsigned int lostPackageResendDelay =0;
    unsigned int MTUSize = DEFAULT_MTU_SIZE;//最大传输单元
    unsigned int maxPackageSize = 0;//包最大字节数
    bool isLostConnect = true;
    time_t lastWinSizeChangeTime = 0;//最近一次窗口调整时间
    time_t lastRecvTime = 0;//最近一次接收包的时间
    time_t lastAskTime = 0;//最近一次接收确认包的时间
    time_t lastSendTime =0;
    unsigned int bytesRecv = 0;
    unsigned int TIME_OUT= 0;
    unsigned int MAX_PACKAGES_SEND_PER_SECOND = 0;
    unsigned int MAX_RECEIVE_PACKAGE_NUM = 0;
    unsigned int lastSecondSendCount = 0;
    unsigned short packageId = 0;
    unsigned int splitId =0;
    queue<InternalPackage*>* output;//输出队列
    queue<InternalPackage*>* askConfirmQueue;//发送确认包队列
    unsigned char waitingSequencedQueue[MAX_ORDER_SIZE];
    unsigned char waitingOrderdQueue[MAX_ORDER_SIZE];
    map<unsigned char, InternalPackage*>* orderedPackageQueue[MAX_ORDER_SIZE];
    time_t recvTime[MAX_RECEIVE_PACKAGE_NUM];
    map<unsigned short, vector<InternalPackage*>*>* splitPackageMap;
    ResendQueue* resendQueue;//重发队列
    queue<InternalPackage*>* sendQueue[NUMBER_OF_PRIORITIES];//发送队列
    InternalPackageManager* internalPackageManager;
    enum {
    };
};

#endif // !TDP_H
