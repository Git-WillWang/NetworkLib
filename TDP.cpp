#include "TDP.h"
//一个应答大小
static const int ASK_BIT_LENGTH= sizeof(unsigned short) * 8 + 1;
//一个UDP包最多包含的应答大小
static const int MAXIMUM_WINDOW_SIZE = (DEFAULT_MTU_SIZE - UDP_HEADER_SIZE) * 8 / ASK_BIT_LENGTH;
//在没有应答的情况下最多发多少个包，resendQueue长度
static const int MINIMUM_WINDOW_SIZE = 10;

TDP::TDP(const unsigned int timeout,const unsigned int maxPackagePerSec) {
   	internalPackageManager = new InternalPackageManager();
    for(int s=0;s<MAX_ORDER_SIZE;++s)
        orderedPackageQueue[s] = new map<unsigned char, InternalPackage*>();
    output = new queue<InternalPackage*>();
    askConfirmQueue = new queue<InternalPackage*>();
    for(int s=0;s<NUMBER_OF_PRIORITIES;++s)
        sendQueue[s] = new queue<InternalPackage*>();
    resendQUeue = new ResendQueue();
    splitPackageMap = new  map<unsigned short, vector<InternalPackage*>>();
    memset(waitingOrderdQueue,0,sizeof(waitingOrderdQueue));
    memset(waitingSequencedQueue,0,sizeof(waitingSequencedQueue));
    memset(recvTime,0,sizeof(recvTime));

    TIME_OUT = timeout;
	MAX_PACKAGES_SEND_PER_SECOND = maxPackagePerSec;
	MAX_RECEIVE_PACKAGE_NUM = timeout / 1000 * maxPackagePerSec;
	windowSize=MINIMUM_WINDOW_SIZE;
	lossyWindowSize=MAXIMUM_WINDOW_SIZE+1; // Infinite
    lostPackageResendDelay = 1000L;
    time(&lastSendTime);
}
TDP::~TDP(){
    free();
}
void TDP::initLog(int timeout,int maxPackagePerSec) {
}
void TDP::free() {
    InternalPackage* p;
    while(!output->empty()){
        p = output->front();
        output->pop();
        internalPackageManager->release(p,true);
    }
    while(!askConfirmQueue->empty()){
        p = askConfirmQueue->front();
        askConfirmQueue->pop();
        internalPackageManager->release(p,true);
    }
    for(int s=0;s<NUMBER_OF_PRIORITIES;++s)
        while(!sendQueue[s]->empty()){
            p = sendQueue[s]->front();
            sendQueue[s]->pop();
            internalPackageManager->release(p,true);
        }

    for(int s=0;s<MAX_ORDER_SIZE;++s){
        map<unsigned char, InternalPackage*>::iterator i;
        for(i =orderedPackageQueue[s]->begin();i!=orderedPackageQueue[s]->end();){
            p = i->value;
            internalPackageManager->release(p,true);
            orderedPackageQueue[s]->erase(i++);
        }
    }
    map<unsigned short, vector<InternalPackage*>*>::iterator i = splitPackageMap->begin();
    while(i!=splitPackageMap->end()){
        vector<InternalPackage*>* v = i->value;
        for(int s=0;s<v->size();++s)
            p = v[s]
    }
    resendQueue->clear();
    memset(recvTime,0,sizeof(recvTime));
    memset(waitingOrderdQueue,0,sizeof(waitingOrderdQueue));
    memset(waitingSequencedQueue,0,sizeof(waitingSequencedQueue));
}

bool TDP::send(BitStream* b, PackagePriority priority, PackageReliability reliability, unsigned char streamIndex) {
	if (b->getBitsUsed() == 0)
		return false;
	time_t now;
	time(&now);
	if (now - lastSendTime <= 2000) {
		++lastSecondSendCount;
		if (lastSecondSendCount >= MAX_AVERAGE_PACKAGE_SEND_PER_SECOND) {
			while (time(NULL) - lastSecondSendCount <= 2000)
				usleep(1000);
		}
	}
	else {
		lastSecondSendCount = 0;
		lastSendTime = now;
	}
	InternalPackage* p = internalPackageManager->getInstance();
	p->createTime = time(NULL);
	p->data = new unsigned char[b->getBytesUsed()];
	memcpy(p->data, b, b->getBytesUsed());
	p->dataBitLength = b->getBitsUsed();
	p->isAskConfirm = false;
	p->nextActTime = 0;
	p->id = packageId;
	if (++packageId == MAX_RECEIVE_PACKAGE_NUM)
		packageId = 0;
	p->priority = priority;
	p->reliability = reliability;
	//包结构：UDP_HEADER-PACKAGE_HEADER-DATA
	unsigned int maxDataSize = maxPackageSize - BITS_TO_BYTES(p->getHeaderLength());
	bool split = BITS_TO_BYTES(p->dataBitLength) > maxDataSize;
	if (split) {
		if (p->reliability == UNRELIABLE)
			p->reliability = RELIABLE;
		else if (p->reliability == UNRELIABLE_SEQUENCED)
			p->reliability = RELIABLE_SEQUENCED;
	}
	if (p->reliability == RELIABLE_SEQUENCED || p->reliability == UNRELIABLE_SEQUENCED) {
		p->streamIndex = streamIndex;
		p->orderIndex = waitingSequencedPackage[streamIndex]++;
	}
	else if (p->reliability == RELIABLE_ORDERED) {
		p->streamIndex = streamIndex;
		p->orderIndex = waitingOrderdPackage[streamIndex]++;
	}
	if (split) {
		splitPackage(p);
		return true;
	}
	sendQueue[p->priority]->push(p);
}
void TDP::splitPackage(InternalPackage* p) {
	unsigned int headerByteLength = BITS_TO_BYTES(p->getHeaderLength());
	unsigned int maxDataSize = maxPackageSize-headerByteLength;
	unsigned int dataByteLength = BITS_TO_BYTES(p->dataBitLength);
	p->splitCount = (unsigned short)((dataByteLength-1) / maxDataSize+1);
	InternalPackage** splitArray = new InternalPackage*[p->splitCount];
	for(int s=0;s<p->splitCount;++s){
		splitArray[s] = internalPackageManager->getInstance();
	}
	unsigned int bytesLeft = dataByteLength;
	for (int s = 0; s < p->splitCount; ++s) {
		if (s == p->splitCount - 1) {
			memcpy(splitArray[s]->data, p->data + s*maxDataSize, dataByteLength - s*maxDataSize);
			splitArray[s]->dataBitLength = (dataByteLength - s*maxDataSize) << 3;
		}
		else {
			memcpy(splitArray[s]->data, p->data + s*maxDataSize, maxDataSize);
			splitArray[s]->dataBitLength = maxDataSize << 3;
		}
		if(s==0)
			splitArray[s]->id = p->id;
		else {
			splitArray[s]->id = packageId++;
			if (packageId == MAX_PACKAGE_NUM)
				packageId = 0;
		}
		splitArray[s]->splitId = splitId;
		splitArray[s]->splitCount = p->splitCount;
		splitArray[s]->splitIndex = s;
	}
	++splitId;
	for (int s = 0; s < p->splitCount; ++s)
		sendQueue[p->priority]->push(splitArray[s]);
	internalPackageManager->realease(p,true);
}
InternalPackage* TDP::createInternalPackage(BitStream* b) {
	if (!b || b->getBitsUsed() < 8)
		return NULL;
	InternalPackage* p = internalPackageManager->getInstance();
	if (!b->read(&p->id)) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	p->id = ntohs(p->id);
	if (!b->read(&p->isAskConfirm)) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	if (p->isAskConfirm)
		return p;
	unsigned char reliability;
	if (!b->readBits(&reliability, 3, true)) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	p->reliability = (PackageReliability)reliability;

	if (p->reliability == UNRELIABLE_SEQUENCED || p->reliability == RELIABLE_SEQUENCED || p->reliability == RELIABLE_ORDERED)
	{
		//32个队列？
		if (!b->readBits((unsigned char*)&(p->streamIndex), 5, true)) {
			internalPackageManager->realease(p,false);
			return NULL;
		}
		if (!b->read(&p->orderIndex)) {
			internalPackageManager->realease(p,false);
			return NULL;
		}
	}
	bool isSplit;
	if (!b->read(&isSplit)) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	if (isSplit) {
		if (!b->read(&p->splitId)) {
			internalPackageManager->realease(p,false);
			return NULL;
		}
		if (!b->read(&p->splitIndex)) {
			internalPackageManager->realease(p,false);
			return NULL;
		}
		if (!b->read(&p->splitCount)) {
			internalPackageManager->realease(p,false);
			return NULL;
		}
		p->splitId = ntohs(p->splitId);
		p->splitIndex = ntohs(p->splitIndex);
		p->splitCount = ntohs(p->splitCount);
	}
	else {
		p->splitId = p->splitCount = 0;
	}
	if (!b->read(&p->dataBitLength)) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	p->dataBitLength = ntohs(p->dataBitLength);
	if (p->dataBitLength <= 0 || BITS_TO_BYTES(p->dataBitLength) >= MAXIMUM_MTU_SIZE) {
		internalPackageManager->realease(p,false);
		return NULL;
	}
	p->data = (unsigned char*)malloc(BITS_TO_BYTES(p->dataBitLength));
	b->readBits(p->data, p->dataBitLength, true);
	return p;
}
//处理接收数据
void TDP::handleRecvData(unsigned char* buffer, int length) {
	if (length <= 1||!buffer)
		return;
	BitStream socketData(buffer, length,false);
	time(&lastRecvTime);
	bytesRecv += length + UDP_HEADER_SIZE;
	InternalPackage* p = createInternalPackage(&socketData);
	while (p) {
		if (p->isAskConfirm) {
			++numOfAskConfirm;
			if (resendQueue.empty())
				lastAskTime = 0;
			else {
				lastAskTime = lastRecvTime;
			}
			resendQueue.erase(p);
		}
		else {
			++numOfRecvPackage;
			if ((p->reliability == RELIABLE_SEQUENCED) ||
				(p->reliability == RELIABLE_ORDERED) ||
				(p->reliability == RELIABLE))
				sendConfirmPackage(p->id);
			if (p->id >= maxNumOfPackageKeep) {
				internalPackageManager->realease(p,true);
				return;
			}
			if (recvTime[p->id] > (lastRecvTime - timeout) || p->orderIndex >= numOfOrderedStream) {
				internalPackageManager->realease(p,true);
			}
			else {
				recvTime[p->id] = lastRecvTime;
				if ((p->reliability == RELIABLE_SEQUENCED || p->reliability == UNRELIABLE_SEQUENCED)) {
					if (p->orderIndex < waitingSequencedPackage[p->streamIndex]) {
						if (p->splitCount > 0) {
							p = buildPackageFromSplitPackageMap(p->id);
							if (p) {
								waitingSequencedPackage[p->streamIndex] = p->orderIndex + 1;
								output.push(p);
								p = NULL;
							}
							else {

							}
						}
						else {
							waitingSequencedPackage[p->streamIndex] = p->orderIndex + 1;
							output.push(p);
							p = NULL;
						}
					}
					else {
						internalPackageManager->realease(p,true);
					}
				}
				else if (p->reliability == RELIABLE_ORDERED) {
					if (p->splitCount > 0)
						p = buildPackageFromSplitPackageMap(p->id);
					if (p) {
						if (waitingOrderdPackage[p->streamIndex] == p->orderIndex) {
							unsigned short streamIndex = p->streamIndex;
							unsigned short orderIndex = p->orderIndex + 1;
							++waitingOrderdPackage[p->streamIndex];
							output.push(p);
							p = NULL;
							map<unsigned char, InternalPackage*>* orderedPackageMap = orderedPackageArray[streamIndex];
							while (!orderedPackageMap->empty()) {
								if ((p = orderedPackageMap->at(orderIndex)) != NULL) {
									orderedPackageMap->erase(++orderIndex);
									output.push(p);
									p = NULL;
								}
								else {
									waitingOrderdPackage[streamIndex] = orderIndex;
									break;
								}
							}
							if (orderedPackageMap->empty())
								orderedPackageArray[streamIndex] = NULL;
						}
						else {
							map<unsigned char, InternalPackage*>* orderedPackageMap = orderedPackageArray[p->streamIndex];
							if (orderedPackageMap == NULL)
								orderedPackageArray[p->streamIndex] = new map<unsigned char, InternalPackage*>(make_pair((unsigned char)0x00, p));
							else
								orderedPackageMap->insert(pair<unsigned char,InternalPackage*>(p->orderIndex, p));
						}
					}
				}
				else if (p->splitCount > 0) {
					p->streamIndex = 255;
					p = buildPackageFromSplitPackageMap(p->id);
				}
			}
		}
		p = createInternalPackage(&socketData);
	}
}


void TDP::mainLoop(SOCKET s, ConnID& connId) {
	time_t now, lastAck = lastAskTime;
	time(&now);
	//重发队列未空，距离上次接收应答时间已经超过超时时间，说明已经断开连接
	if (resendQueue.size() > 0 && lastAck && now - lastAck > timeout) {
		isLostConnect = true;
		return;
	}
	if (needToSend()) {
        BitStream b;
		do {
            b.reset();
			generateFrame(&b);
			if(b->getBitsUsed>8)
				sendBitStream(s,connId,b);
		}while(b.getBitsUsed()>8 && needToSend());
	}
}
void TDP::sendBitStream(SOCKET s,ConnID& connId,BitStream* b){
    ++bitStreamSent;
    Socket::getInstance()->send(s,(char*)b->getData(),b->getBitsUsed(),connId);
    time(&lastSendTime);
}
bool TDP::needToSend() {
	//等待重发队列长度已经超过窗口大小
	if (!isResendQueueOverflow())
		return true;
	time_t now;
	time(&now);
	if (askConfirmQueue.size() >= MINIMUM_WINDOW_SIZE && askConfirmQueue.front()->nextActTime < now) {
		return true;
	}
	if (resendQueue.size() > 0 && resendQueue.front() && resendQueue.front()->nextActTime < now) {
		return true;
	}
	return false;
}
bool TDP::isResendQueueOverflow() {
	unsigned win = windowSize;
	return resendQueue.size() >= win;
}

void TDP::generateFrame(BitStream* output, bool& isReliablePackageSent) {
	time_t now;
	InternalPackage* p = NULL;
	time(&now);
	isReliablePackageSent = false;
	bool isAskConfirmPackageSent = false;
	bool hasPackageLost = false;
	int maxDataBitSize = maxPackageSize << 3;
	if (askConfirmQueue.size() > 0 && (askConfirmQueue.size() >= MINIMUM_WINDOW_SIZE || askConfirmQueue.front()->nextActTime < now)) {
		do {
            if(output->getBitsUsed()+ASK_BIT_LENGTH > maxDataBitSize)
                return;
			p = askConfirmQueue.front();
			askConfirmQueue.pop();
			internalPackageManager->realease(p,false);
			writeToBitStream(output,p);
			isAskConfirmPackageSent = true;
		} while (!askConfirmQueue.empty());
	}
	//重发队列中有包且包的下次重发时间已经到达
	while (resendQueue.size() > 0 && resendQueue.front()->nextActTime < now) {
		p = resendQueue.front();
		unsigned int nextPackageLength = p->getHeaderLength() + p->dataBitLength;
		//发现resendQueue的内容写到一个bitStream中还有剩，减少windowSize大小
		if (dataStream->getBitsUsed() + nextPackageLength > maxDataBitSize) {
			if (hasPackageLost) {
				if (--windowSize <= MINIMUM_WINDOW_SIZE)
					windowSize = MINIMUM_WINDOW_SIZE;
				lossyWindowSize = windowSize;
				lastWinSizeChangeTime = now;
			}
			return;
		}
		++numOfPackageSent;
		writeToBitStream(output, p);
		isReliablePackageSent = true;
		++resendPackageSent;
		resendQueue.pop();
		p->nextActTime = now + lostPackageResentDelay;
		resendQueue.push(p);
	}
	for (int i = 0; i < NUMBER_OF_PRIORITIES; ++i) {
		while (!sendQueue[i]->empty()) {
			p = sendQueue[i]->front();
			unsigned int nextPackageLength = p->getHeaderLength() + p->dataBitLength;
			if (output->getBitsUsed() + nextPackageLength > maxDataBitSize) {
				break;
			}
			sendQueue[i]->pop();
			++numOfPackageSent;
			writeToBitStream(output, p);
			if (p->reliability == RELIABLE || p->reliability == RELIABLE_SEQUENCED || p->reliability == RELIABLE_ORDERED) {
				p->nextActTime = now + lostPackageResentDelay;
				resendQueue.push(p);
			}
			else {
				internalPackageManager->realease(p,true);
			}
		}
	}
    //剩下的还有空间，如果确认队列未空，则将剩下的发送
    while (output->getBitsUsed() + ASK_BIT_LENGTH < maxDataBitSize && askConfirmQueue.size()>0) {
        p = askConfirmQueue.front();
        askConfirmQueue.pop();
        writeToBitStream(output,p);
        internalPackageManager->realease(p,true);
    }
}

void TDP::writeToBitStream(BitStream* b, const InternalPackage* const p) {
	b->write(htons(p->id));
	b->write(p->isAskConfirm);
	if(p->isAskConfirm)
		return;
	unsigned char reliability = (unsigned char)p->reliability;
	b->writeBits(&reliability, 3,true);
	if (p->reliability == UNRELIABLE_SEQUENCED || p->reliability == RELIABLE_SEQUENCED || p->reliability == RELIABLE_ORDERED) {
		b->writeBits((unsigned char*)&p->streamIndex, 5, true);
		b->write(p->orderIndex);
	}
	bool isSplit = p->splitCount > 0;
	b->write(isSplit);
	if(isSplit) {
		b->write(htons(p->splitId));
		b->write(htons(p->splitIndex));
		b->write(htons(p->splitCount));
	}
	unsigned short length = p->dataBitLength;
	b->write(htons(p->dataBitLength));
	b->writeBits(p->data, p->dataBitLength, true);
}
void TDP::setMTU(const unsigned int size){
    if(size >= MAXIMUM_MTU_SIZE)
        MTUSize = MAXIMUM_MTU_SIZE;
    else MTUSize = size;
    maxPackageSize = MTUSize - UDP_HEADER_SIZE;
}
unsigned int TDP::getMTU(){
    return MTUsize;
}
void TDP::sendConfirmPackage(unsigned int id){
    InternalPackage* p = InternalPackageManager.getInstance();
    p->id = id;
    p->isAskConfirm = true;
    time(&p->createTime);
    p->nextActTime = p->createTime + lostPackageResendDelay/4;
    askConfirmQueue.push(p);
}
