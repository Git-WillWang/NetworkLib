#include "UDPSocket.h"
//һ��Ӧ���С
static const int ASK_BIT_LENGTH= sizeof(unsigned short) * 8 + 1;
//һ��UDP����������Ӧ���С
static const int MAXIMUM_WINDOW_SIZE = (DEFAULT_MTU_SIZE - UDP_HEADER_SIZE) * 8 / ASK_BIT_LENGTH;
//��û��Ӧ����������෢���ٸ�����resendQueue����
static const int MINIMUM_WINDOW_SIZE = 10;
using namespace NetworkLib;

UDPSocket::UDPSocket() {
	internalPackageManager = new InternalPackageManager();

}
void UDPSocket::initLog(int timeout,int maxPackagePerSec) {
	this->timeout = timeout;
	this->maxPackagePerSec = maxPackagePerSec;
	maxNumOfPackageKeep = timeout / 1000 * maxPackagePerSec;
}
void UDPSocket::free() {

}

bool UDPSocket::send(BitStream* b, PackagePriority priority, PackageReliability reliability, unsigned char streamIndex) {
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
	//���ṹ��UDP_HEADER-PACKAGE_HEADER-DATA
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
void UDPSocket::splitPackage(InternalPackage* p) {
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
InternalPackage* UDPSocket::createInternalPackage(BitStream* b) {
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
		//32�����У�
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
//�����������
void UDPSocket::handleRecvData(unsigned char* buffer, int length) {
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


void UDPSocket::mainLoop(SOCKET s, ConnID id, int MTUSize) {
	time_t now, lastAck = lastAskTime;
	time(&now);
	//�ط�����δ�գ������ϴν���Ӧ��ʱ���Ѿ�������ʱʱ�䣬˵���Ѿ��Ͽ�����
	if (resendQueue.size() > 0 && lastAck && now - lastAck > timeout) {
		isLostConnect = true;
		return;
	}
	if (needToSend()) {
		do {
			generateFrame(dataStream, MTUSize, );
			if(dataStream->getBitsUsed>8)
				sendBitStream(s,id,dataStream);
		}while(dataStream.getBitsUsed()>8 && needToSend());
	}
}
void UDPSocket::sendBitStream(SOCKET s,ConnID connId,BitStream* b){
    ++bitStreamSent;
    Socket::getInstance()->send(s,(char*)b->getData(),b->getBitsUsed(),connId);
    time(&lastSendTime);
}
bool UDPSocket::needToSend() {
	//�ȴ��ط����г����Ѿ��������ڴ�С
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
bool UDPSocket::isResendQueueOverflow() {
	unsigned win = windowSize;
	return resendQueue.size() >= win;
}

void UDPSocket::generateFrame(BitStream* output, int MTUSize, bool& isReliablePackageSent) {
	time_t now;
	InternalPackage* p = NULL;
	time(&now);
	isReliablePackageSent = false;
	bool isAskConfirmPackageSent = false;
	bool hasPackageLost = false;
	int maxDataBitSize = (MTUSize - UDP_HEADER_SIZE) << 3;
	if (askConfirmQueue.size() > 0 && (askConfirmQueue.size() >= MINIMUM_WINDOW_SIZE || askConfirmQueue.front()->nextActTime < now)) {
		do {
			p = askConfirmQueue.front();
			askConfirmQueue.pop();
			internalPackageManager->realease(p,false);
			isAskConfirmPackageSent = true;
			if (dataStream->getBitsUsed() > maxDataBitSize)
				return;
		} while (!askConfirmQueue.empty());
	}
	while (resendQueue.size() > 0 && resendQueue.front()->nextActTime < now) {
		p = resendQueue.front();
		unsigned int nextPackageLength = p->getHeaderLength() + p->dataBitLength;
		//����resendQueue������д��һ��bitStream�л���ʣ������windowSize��С
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
	//���֮ǰû���͹�ȷ�ϰ������ǻ��пռ䣬��ô����
	if (isAskConfirmPackageSent == false && output->getBitsUsed() > 0) {
		while (output->getBitsUsed() + ASK_BIT_LENGTH < maxDataBitSize && askConfirmQueue.size()>0) {
			p = askConfirmQueue.front();
			askConfirmQueue.pop();
			writeToBitStream(output,p);
			internalPackageManager->realease(p,true);
		}
	}
}

void UDPSocket::writeToBitStream(BitStream* b, const InternalPackage* const p) {
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
void UDPSocket::setMTU(const unsigned int size){
    if(size >= MAXIMUM_MTU_SIZE)
        MTUSize = MAXIMUM_MTU_SIZE;
    else MTUSize = size;
    maxPackageSize = MTUSize - UDP_HEADER_SIZE;
}
unsigned int UDPSocket::getMTU(){
    return MTUsize;
}
void UDPSocket::sendConfirmPackage(unsigned int id){
    InternalPackage* p = InternalPackageManager.getInstance();
    p->id = id;
    p->isAskConfirm = true;
    time(&p->createTime);
    p->nextActTime = p->createTime + lostPackageResendDelay/4;
    askConfirmQueue.push(p);
}
