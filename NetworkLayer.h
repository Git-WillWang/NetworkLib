#pragma once
#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H
#include "BitStream.h"
#include "NetworkLibType.h"
#include "UDPSocket.h"
#include <thread>
#include <sys/types.h>
class Pipe {
	ConnID connId;
	UDPSocket transfer;
	bool weInitiatedTheConnection;
    PingAndClockDifferential pingAndClockDifferential[PING_TIMES_ARRAY_SIZE];
	int pingAndClockDifferentialWriteIndex;
	int lowestPing;
	time_t nextPingTime;
	BitStream staticData;
	time_t connTime;
};
#define CONNECTION_REQUEST 0x00
class NetworkLayer {
public:
	NetworkLayer();
	virtual ~NetworkLayer();
	//! ����Զ������
	virtual bool connect(char* host, unsigned short remotePort, unsigned long ulSessionID = 0);
	//! �Ƿ��Ѿ���������
	virtual bool isConnected(ConnID& ConnectID);
	//! �ر����е�����
	virtual void disconnect(void);
	//!�رյ�������ͨ��
	virtual void disconnect(ConnID& target, bool sendDisconNote);
	//! ��������
	virtual bool sendPackage(ConnID& ConnectID, char *pData, const long lLength, PackageReliability reliability = RELIABLE_ORDERED);
	//! �������ݰ�
	virtual Package* ReceivePackage();
	//! �������ݰ�������Receive�õ������ݰ�����Ҫ���ñ���������
	virtual void DestoryPackage(Package* pPackage);
	//! ȡ��ƽ��ping��ʱ��
	//virtual int GetAveragePing(ConnID& target) const;
	//! ȡ����һ��ping��ʱ��
	//virtual int GetLastPing(ConnID& target) const;
	//! ȡ������һ��ping��ʱ��
	//virtual int GetLowestPing(ConnID& target) const;
	//-------------------------------------------------------------------------------------------------------------------------------------
	//! ��ʼ�������
	bool initialize(unsigned short maxNumOfPipes, unsigned short localPort, bool useAnyPort = false, bool onlyLocal = false, int timeout = 10000, int maxPacketsPerSecond = 400);
	//! �õ�ͨ�����������
	unsigned short GetMaximumNumberOfPeers(void) const { return maximumNumberOfPeers; }
	//! �����������ӵ��������
	void SetMaximumIncomingConnections(unsigned short numberAllowed) { maximumIncomingConnections = numberAllowed; }
	//! ȡ�����ӵ��������
	unsigned short GetMaximumIncomingConnections(void) const;
	//! �Ƿ��ڻ״̬
	bool IsActive(void) const;
	//! ȡ����������
	bool GetConnectionList(ConnID& *remoteSystems, unsigned short *numberOfSystems) const;
	//! ͨ��ConnID& �õ����ڵ�����
	int GetIndexFromPlayerID(ConnID& playerId);
	//! ͨ�������õ�ConnID&
	ConnID& GetPlayerIDFromIndex(int index);
	bool send(char *data, const long length, PackagePriority priority, PackageReliability reliability,unsigned char streamIndex, ConnID& playerId);
	bool send(BitStream* b, PackagePriority priority, PackageReliability reliability,unsigned char streamIndex, ConnID& playerId);
	//! Ping Զ��ϵͳ
	void Ping(ConnID& target);
	//! ȡ��ͨ���б�
	Pipe* getPipeList() { return pipeList; }
	//! ͨ������ID�õ�ͨ��
	Pipe* getPipe(ConnID& connId) const;
	//! ������ѭ���Ƿ��ڻ״̬��
	void SetMainLoopThreadActive(bool bActive) { isMainLoopThreadActive = bActive; }
	//! ���ý���ѭ���Ƿ��ڻ״̬��
	void SetRcvPackageThreadActive(bool bActive) { isRecvfromThreadActive = bActive; }
	//! �����Ƿ�����߳�
	void SetEndThread(bool bEnd) { endThreads = bEnd; }
	//! �Ƿ�����߳�����
	bool IsEndThread() { return endThreads; }
	//! ȡ�ü���Socket
	SOCKET GetListenSocket() { return connectionSocket; }
	//! ������������
	void handleConnRequest(ConnID& connId, unsigned long sessionId);
	//! �����ӷ���ͨ��
	Pipe * assignPipe(ConnID& connId);
	//! ��ʼ��ͨ������
	void ResetRemoteSystemData(Pipe* remoteSystem, bool weInitiatedTheConnection);
	//! ȡ��MTUSize
	int GetMTUSize() { return MTUSize; }
	//! �Ƿ��ǿͻ���
	bool m_bItIsClient;
	void RecQueuePush(Package* p);
private:
	//! �Ƿ��ʼ��
	bool m_bInitNetLayer;
	//! ���������ID
	ConnID& id;
	//! ����Socket
	SOCKET listenSocket;
	//! ���ͨ������
	unsigned short maxNumOfPipes;
	//! ͨ���б�
	Pipe* pipeList;
	unsigned int numOfPipes;
	//! �����������
	unsigned short maxNumOfConnections;
	//! �߳��Ƿ�ֹͣ
	bool threadsStop;
	//! ��ѭ���߳��Ƿ��Ѿ�����
	bool isMainLoopThreadActive;
	//!	�����߳��Ƿ�����
	bool isRecThreadActive;
	//! ���͵�������
	unsigned long bytesSent;
	//! ���ܵ�������
	unsigned long bytesRecieved;
	//! ����С
	int MTUSize;
	queue<Package*> RecQueue;
};
#endif // !NETWORKLAYER_H
