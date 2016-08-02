#ifndef NETWORKLAYER_H
#define NETWORKLAYER_H
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
#endif // !NETWORKLAYER_H
