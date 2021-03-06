#pragma once
#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H
#include "BitStream.h"
#include "NetworkLibType.h"
#include "TDP.h"
#include <thread>
#include <sys/types.h>
class Pipe {
	ConnID connId;
	TDP tdp;
	bool weInitiatedTheConnection;
    PingAndClockDifferential pingAndClockDifferential[PING_TIMES_ARRAY_SIZE];
	int pingAndClockDifferentialWriteIndex;
	int lowestPing;
	time_t nextPingTime;
	BitStream staticData;
	time_t connTime;
};
#define CONNECTION_REQUEST 0x00
const ConnID UNASSIGNED_CONNID = {0xFFFFFFFF,0xFFFF};
class NetworkLayer {
public:
	NetworkLayer();
	virtual ~NetworkLayer();
	//! 连接远程主机
	virtual bool connect(char* host, unsigned short remotePort, unsigned long ulSessionID = 0);
	//! 是否已经存在连接
	virtual bool isConnected(ConnID& ConnectID);
	//! 关闭所有的连接
	virtual void disconnect(void);
	//!关闭单个连接通道
	virtual void disconnect(ConnID& target, bool sendDisconNote);
	//! 发送数据
	virtual bool sendPackage(ConnID& ConnectID, char *pData, const long lLength, PackageReliability reliability = RELIABLE_ORDERED);
	//! 接收数据包
	virtual Package* ReceivePackage();
	//! 销毁数据包，调用Receive得到的数据包，需要调用本函数销毁
	virtual void DestoryPackage(Package* pPackage);
	//! 取得平均ping的时间
	//virtual int GetAveragePing(ConnID& target) const;
	//! 取得上一次ping的时间
	//virtual int GetLastPing(ConnID& target) const;
	//! 取得最慢一次ping的时间
	//virtual int GetLowestPing(ConnID& target) const;
	//-------------------------------------------------------------------------------------------------------------------------------------
	//! 初始化网络层
	bool initialize(unsigned short maxNumOfPipes, unsigned short localPort, bool useAnyPort = false, bool onlyLocal = false, int timeout = 10000, int maxPacketsPerSecond = 400);
	//! 得到通道的最大数量
	unsigned short getMaxNumOfPipes(void) const { return maximumNumberOfPeers; }
	//! 设置允许连接的最大数量
	void SetMaximumIncomingConnections(unsigned short numberAllowed) { maximumIncomingConnections = numberAllowed; }
	//! 取得连接的最大数量
	unsigned short GetMaximumIncomingConnections(void) const;
	//! 是否处于活动状态
	bool IsActive(void) const;
	//! 取得所有连接
	bool GetConnectionList(ConnID& *remoteSystems, unsigned short *numberOfSystems) const;
	//! 通过ConnID& 得到所在的索引
	int GetIndexFromPlayerID(ConnID& playerId);
	//! 通过索引得到ConnID&
	ConnID& GetPlayerIDFromIndex(int index);
	bool send(char *data, const long length, PackagePriority priority, PackageReliability reliability,unsigned char streamIndex, ConnID& playerId);
	bool send(BitStream* b, PackagePriority priority, PackageReliability reliability,unsigned char streamIndex, ConnID& playerId);
	//! Ping 远程系统
	void Ping(ConnID& target);
	//! 取得通道列表
	Pipe* getPipeList() { return pipeList; }
	//! 通过连接ID得到通道
	Pipe* getPipe(ConnID& connId) const;
	//! 设置主循环是否处于活动状态中
	void SetMainLoopThreadActive(bool bActive) { isMainLoopThreadActive = bActive; }
	//! 设置接收循环是否处于活动状态中
	void SetRcvPackageThreadActive(bool bActive) { isRecvfromThreadActive = bActive; }
	//! 设置是否结束线程
	void SetEndThread(bool bEnd) { endThreads = bEnd; }
	//! 是否结束线程运行
	bool IsEndThread() { return endThreads; }
	//! 取得监听Socket
	SOCKET GetListenSocket() { return connectionSocket; }
	//! 处理连接请求
	void handleConnRequest(ConnID& connId, unsigned long sessionId);
	//! 给连接分配通道
	Pipe * assignPipe(ConnID& connId);
	//! 初始化通道变量
	void ResetRemoteSystemData(Pipe* remoteSystem, bool weInitiatedTheConnection);
	//! 取得MTUSize
	int GetMTUSize() { return MTUSize; }
	//! 是否是客户端
	bool m_bItIsClient;
	void RecQueuePush(Package* p);
private:
	//! 是否初始化
	bool m_bInitNetLayer;
	//! 本身的连接ID
	ConnID& id;
	//! 监听Socket
	SOCKET listenSocket;
	//! 最大通道数量
	unsigned short maxNumOfPipes;
	//! 通道列表
	Pipe* pipeList;
	unsigned int numOfPipes;
	//! 最大连接数量
	unsigned short maxNumOfConnections;
	//! 线程是否停止
	bool allThreadsStop;
	//! 主循环线程是否已经启动
	bool isMainLoopThreadActive;
	//!	接收线程是否启动
	bool isRecvThreadActive;
	//! 发送的数据量
	unsigned long bytesSent;
	//! 接受的数据量
	unsigned long bytesRecieved;
	//! 包大小
	int MTUSize;
	queue<Package*> RecvQueue;
};
#endif // !NETWORKLAYER_H
