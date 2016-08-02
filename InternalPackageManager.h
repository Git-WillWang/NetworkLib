#pragma once
#ifndef INTERNALPACKAGEMANAGER_H
#define INTERNALPACKAGEMANAGER_H
#define DEFAULT_PACKAGE_NUM 100
#define MAX_PACKAGE_NUM 1000
#include<queue>
#include "NetworkLibType.h"
using namespace std;

class InternalPackage {
public:
	InternalPackage(){}
	InternalPackage(int size) {
		data = new unsigned char[size];
	}
	~InternalPackage() {
		delete[] data;
	}
	unsigned short getHeaderLength() {
		unsigned short len;
		//id+1λ
		if (isAskConfirm)
			return ASK_BIT_LENGTH;
		len += 4;
		if (reliability == UNRELIABLE_SEQUENCED || reliability == RELIABLE_SEQUENCED || reliability == RELIABLE_ORDERED) {
			len += 13;
		}
		//splitID,splitIndex,splitCount
		bool isSplit = splitCount > 0;
		if (isSplit)
			len += 3 * 16;
		len += sizeof(unsigned int)*8;
		return len;
	}
public:
	bool isAskConfirm;
	unsigned short id;
	PackagePriority priority;
	PackageReliability reliability;
	unsigned char streamIndex;
	unsigned char orderIndex;
	unsigned short splitId;
	unsigned short splitIndex;
	unsigned short splitCount;
	time_t createTime;
	time_t nextActTime;
	unsigned int dataBitLength;
	unsigned char index;
	unsigned char* data;
};

class InternalPackageManager {
public:
	InternalPackageManager();
	InternalPackageManager(int);
	~InternalPackageManager();
	void realease(InternalPackage*,bool);
	InternalPackage* getInstance();
	bool extend(int num);
private:
	int size;
	queue<InternalPackage*> pool;

};
#endif // !__INTERNALPACKAGEMANAGER_H