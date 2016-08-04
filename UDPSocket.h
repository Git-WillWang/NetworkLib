#pragma once
#ifndef __NETWORKLIB_UDPSOCKET_INCLUDE__
#define __NETWORKLIB_UDPSOCKET_INCLUDE__
#include "NetworkLibType.h"
#include "InternalPackageManager.h"
#include "BitStream.h"
#include <list>
#include <map>
#include <time.h>
#include <sys/socket.h>
using namespace std;
namespace NetworkLib {
#define UDP_HEADER_SIZE 28 //包头大小
#ifndef DEFAULT_MTU_SIZE
#define DEFAULT_MTU_SIZE 576 //默认数据长度
#define MAXIMUM_MTU_SIZE 8000 //最大数据长度
#define MAX_ORDER_SIZE 255
#endif // !DEFAULT_MTU_SIZE

	class UDPSocket {
	public:
		UDPSocket();
		~UDPSocket();
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
		class ResendQueue {
		public:
			ResendQueue() {
				Queue = new map<unsigned short, InternalPackage*>();
				TimeIdMap = new map<time_t, list<unsigned short>*>();
			}
			void push(InternalPackage* p) {
				if (!p) return;
				Queue->insert(make_pair(p->id, p));
				list<unsigned short>* l = TimeIdMap->at(p->nextActTime);
				l->push_back(p->id);
			}
			void pop() {
				if (!Queue->empty()) {
					list<unsigned short>* l = TimeIdMap->begin()->second;
					time_t time = TimeIdMap->begin()->first;
					unsigned short id = l->back();
					l->pop_back();
					//list中已经没有数据，删除TimeIdMap中的list节点
					if (l->empty())
						TimeIdMap->erase(time);
					Queue->erase(id);
				}
			}
			void erase(InternalPackage* p) {
				list<unsigned short>* l = TimeIdMap->at(p->nextActTime);
				list<unsigned short>::iterator i = l->begin();
				while (i != l->end()) {
					if (*i == p->id) {
						l->erase(i);
						break;
					}
					++i;
				}
				if (l->empty())
					TimeIdMap->erase(p->nextActTime);
				Queue->erase(p->id);
			}
			bool empty() {
				return Queue->empty();
			}
			int size() {
				return Queue->size();
			}
			InternalPackage* front() {
				if (Queue->empty())
					return NULL;
				unsigned short id = TimeIdMap->begin()->second->back();
				return Queue->at(id);
			}
		private:
			map<unsigned short, InternalPackage*>* Queue;
			map<time_t, list<unsigned short>*>* TimeIdMap;
		};

		unsigned int timeout;
		unsigned int maxPackagePerSec;
		unsigned int numOfAskConfirm;
		unsigned int numOfRecvPackage;
		unsigned int numOfOrderedStream;
		unsigned int maxNumOfPackageKeep;
		unsigned int windowSize;
		unsigned int lossyWindowSize;
		unsigned int numOfPackageSent;
		unsigned int bitStreamSent;
		unsigned int resendPackageSent;
		unsigned int lostPackageResendDelay;
		unsigned int MTUSize;//最大传输单元
		unsigned int maxPackageSize;//包最大字节数
		bool isLostConnect;
		BitStream* dataStream;
		time_t lastWinSizeChangeTime;//最近一次窗口调整时间
		time_t lastRecvTime;//最近一次接收包的时间
		time_t lastAskTime;//最近一次接收确认包的时间
		time_t lastSendTime;
		unsigned int bytesRec;
		unsigned int MAX_AVERAGE_PACKAGE_SEND_PER_SECOND;
		unsigned int MAX_RECEIVE_PACKAGE_NUM;
		unsigned int lastSecondSendCount;
        unsigned short packageId;
		unsigned int splitId;
		queue<InternalPackage*> output;//输出队列
		queue<InternalPackage*> askConfirmQueue;//发送确认包队列
		vector<unsigned short> waitingSequencedPackage;
		vector<unsigned short> waitingOrderdPackage;
		map<unsigned char, InternalPackage*>* orderedPackageArray[MAX_ORDER_SIZE] = { NULL };
		vector<time_t> recvTime;
		map<unsigned short, vector<InternalPackage*>> splitPackageMap;
		ResendQueue resendQueue;//重发队列
		queue<InternalPackage*>* sendQueue[4];//发送队列
		InternalPackageManager* internalPackageManager;
		enum {

		};

	};
}

#endif // !__NETWORKLIB_UDPSOCKET_H
