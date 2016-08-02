#pragma once
#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H
#include "BitStream.h"
#include "NetworkLibType.h"
#include "UDPSocket.h"
#include <thread>
#include <sys/types.h>
class Pipe {
	ConnID clientID;
	UDPSocket reLayer;
	bool a;
};
class NetworkLayer {
public:
	NetworkLayer();
	virtual ~NetworkLayer();
	//! ����Զ������
	virtual bool connect(unsigned char* host, unsigned short remotePort, unsigned long ulSessionID = 0);
	//! �Ƿ��Ѿ���������
	virtual bool isConnected(ConnID& ConnectID);
	//! �ر����е�����
	virtual void disconnect(void);
	//! ��������
	virtual bool sendPackage(ConnID& ConnectID, char *pData, const long lLength, PackageReliability reliability = RELIABLE_ORDERED);
	//! �������ݰ�
	virtual Package* ReceivePackage();
	//! �������ݰ�������Receive�õ������ݰ�����Ҫ���ñ���������
	virtual void DestoryPackage(Package* pPackage);
	//! �ر�����
	virtual void CloseConnection(ConnID& target, bool sendDisconnectionNotification);
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
	bool send(unsigned char *data, const long length, PackagePriority priority, PackageReliability reliability, char orderingStream, ConnID& playerId);
	bool send(BitStream* b, PackagePriority priority, PackageReliability reliability, char orderingStream, ConnID& playerId);
	//! Ping Զ��ϵͳ
	void Ping(ConnID& target);
	//! ȡ��ͨ���б�
	Pipe* GetRemoteSystemList() { return remoteSystemList; }
	//! ͨ������ID�õ�ͨ��
	Pipe* GetRemoteSystemFromConnectID(ConnID& ConnectID) const;
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
	void ProcessConnectionRequest(ConnID& ConnectId, unsigned long ulSessionID);
	//! �����ӷ���ͨ��
	Pipe * SetRemoteSystemFromConnectID(ConnID& ConnectId);
	//! ��ʼ��ͨ������
	void ResetRemoteSystemData(Pipe* remoteSystem, bool weInitiatedTheConnection);
	//! ȡ��MTUSize
	int GetMTUSize() { return MTUSize; }
	//! �Ƿ��ǿͻ���
	bool m_bItIsClient;
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
};
#endif // !NETWORKLAYER_H
