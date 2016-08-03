#include "NetworkLayer.h"
NetworkLayer::NetworkLayer() {
	maxNumOfPipes = 0;
	maxNumOfConnections = 0;
	pipeList = NULL;
	isMainLoopThreadActive = false;
	isRecThreadActive = false;
	MTUSize = DEFAULT_MTU_SIZE;
}

bool NetworkLayer::initialize(unsigned short maxNumOfPipes, unsigned short localPort, bool useAnyPort = false, bool onlyLocal = false, int timeout = 10000, int maxPackagePerSec = 400) {
	if (listenSocket == INVALID_SOCKET) {
		if ((listenSocket = Socket::getInstance()->createListenSocket(localPort, useAnyPort, onlyLocal)) == INVALID_SOCKET)
			return false;
	}
	if(maxNumOfPipes==0){
		pipeList = new Pipe[maxNumOfPipes];
		for (int i = 0; i < maxNumOfPipes; ++i) {
			pipeList[i].reliabilityLayer->initLog(timeout, maxPackagesPerSec);
			pipeList[i].id = UNASSIGNED_ID;
		}
		this->maxNumOfPipes = maxNumOfPipes;
		if (maxNumOfConnections > maxNumOfPipes)
			maxNumOfConnections = maxNumOfPipes;
	}
	if (threadsStop) {
		bytesSents = 0;
		bytesRecieved = 0;
		threadsStop = false;
		id->port = localPort;
		id->ip = inet_addr();
		pthread_t processPackageThread = NULL;
		pthread_t recThread = NULL;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		int error;
		if (!isMainLoopThreadActive) {
			error = pthread_create(&processPackageThread, &attr, &UpdateNetworkLoop, this);
			if (error) {
				disconnect();
				return false;
			}
		}
		if (!isRecThreadActive) {
			error = pthread_create(&recThreadPackage, &attr, &RecvFromNetworkLoop, this);
			if (error)
			{
				disconnect();
				return false;
			}
		}
		pthread_attr_destory(&attr);
		processPackageThread = NULL;
		while (isRecThreadActive == false || isMainLoopThreadActive == false) {
			usleep(10000);
		}
	}
	isInitNetworkLayer = true;
 	teturn true;
}
void NetworkLayer::disconnect(){
    for(int s=0;s<maxNumOfPipes;++s){
        unsigned char c  = DISCONNECT_NOTIFICATION;
        send((char*)&c,sizeof(c),HIGH_PRIORITY,UNRELIABLE,0,target);
        pipeList[i].transfer.mainLoop(listenSocket,target,MTUSize);
        pipeList[i].connId = UNASSIGNED_CONNECT_ID;
        pipeList[i].transfer.reset();
    }
    maxNumOfPipes  =0;
    threadsStop = true;
    while(isMainLoopThreadActive){
        usleep(10*1000);
    }
	char c=0;
	Socket::getInstance()->send(listenSocket, &c, 1, "127.0.0.1", myPlayerId.usPort);
	while(isRecThreadActive)
		usleep(10 * 1000);
	if (listeSocket != INVALID_SOCKET)
	{
		closesocket(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	// Clear out the queues
	while (!RecQueue.empty())
	{
        PackageManager::realease(RecQueue.front());
        RecQueue.pop();
	}

    Pipe* temp = pipeList;
	remoteSystemList= NULL;
	delete [] temp;
}
void NetworkLayer::disconnect(ConnID& target,bool sendDisconnNote){
    if(!pipList||!threadStop)
        return;
    if(sendDisconnNote){
        unsigned char c = DISCONNECT_NOTIFICATION;
        send((char*)&c,sizeof(c),HIGH_PRIORITY,UNRELIABLE,0,target);
    }
    for(int i =0;i<maxNumOfPipes;++i){
        if(pipeList[i].connId == target){
            pipeList[i].transfer.mainLoop(listenSocket,target,MTUSize);
            pipeList[i].connId = UNASSIGNED_CONNECT_ID;
            pipeList[i].transfer.reset();
            --numOfConnections;
            break;
        }
    }
}
bool NetworkLayer::connect(char* host, unsigned short remotePort, unsigned long sessionId = 0){
    if(!isInitNetworkLayer)
        return false;
    char ip[64];
    if(!Socket::hostnameToIp(host,ip)){
        return false;
    }
    BitStream b;
    unsigned char c = (unsigned char)CONNECTION_REQUEST;
    b.write(&c);
    ConnID connId(inet_addr(ip),remotePort);
    Pipe* pipe = getPipe(connId);
    while(pipe){
        Package* package = PackageManager::getInstance();
        package->data = new char[1];
        package->data[0] = CONNECTION_LOST;
        package->connId = connId;
        package->dataLength = sizeof(char);
        RecQueuePush(package);
        disconnect(connId,false);
        pipe = getPipe(connId);
    }
}
void NetworkLayer::RecQueuePush(Package* p){
    if(!p)
        RecQueue.push(p);
}
Pipe* NetworkLayer::getPipe(ConnID& connId) const{
    for(int s=0;s<numOfPipes;++s){
        if(pipeList[s].connId == connId){
            return &pipeList[s];
            break;
        }
    }
    return NULL;
}
Pipe* NetworkLayer::assignPipe(ConnID& connId){
    Pipe* pipe = NULL;
    for(int s=0;s<maxNumOfPipes;++s){
        if(pipeList[s].connId == connId){
            pipeList[s].transfer.mainLoop(listenSocket,connId,MTUSize);
            pipeList[s].transfer.reset();
            pipeList[s].connId = connId;
            break;
        }
    }
    if(!pipe){
        for(int s=0;s<maxNumOfPipes;++s){
            if(pipeList[s] == UNASSIGNED_PLAYER_ID){
                pipeList[s].connId = connId;
				pipe = &pipeList[s];
				break;
            }
        }
    }
    return pipe;
}
void NetworkLayer::handleConnRequest(ConnID& connId, unsigned long sessionId){
    Pipe* pipe = assignPipe(connId);
    if(!pipe){
        unsigned char c = NO_MORE_PIPE;
        Socket::getInstance().send(listenSocket,(char*)&c,sizeof(c),connId);
        return;
    }
    resetPipe(pipe);
    ConnAcceptPackage cap;
    cap.PackageId = CONNECTION_REQUEST_ACCEPTED;
    cap.connId.ip = htonl(connId.ip);
    cap.connId.port = htonl(connId.port);
    cap.sessionId = sessionId;
    if(0!=Socket::send(listenSocket,(char*)&cap,sizeof(cap),connId)){
        disconnect(connId,false);
        return;
    }
    Package* p = PackageManager::getInstance();
    p->data = new unsigned char[1];
    p->data[0] = ID_INCOMING_CONNECTION;
    p->dataLength = sizeof(char);
    p->connID = connId;
    RecQueuePush(p);
}
bool NetworkLayer::send(char *data, const long length, PackagePriority priority, PackageReliability reliability, unsigned char streamIndex, ConnID& connId){
    if(!data|| length<0) return false;
    BitStream b(data,length,false);
    return send(b,priority,reliability,streamIndex,connId);
}
bool NetworkLayer::send(BitStream* b, PackagePriority priority, PackageReliability reliability,unsigned char streamIndex, ConnID& connId){
    if(b->getBitsUsed() == 0||!pipeList||!threadsStop)
        return false;
    Pipe* pipe = getPipe(connId);
    if(pipe){
        bytesCent += b->getBytesUsed();
        pipe->transfer.send(b,priority,reliability,streamIndex,MTUSize);
        return true;
    }
    return false;
}
#endif // !NETWORKLAYER_H














