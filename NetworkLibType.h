#pragma once
#ifndef NETWORKLIBTYPE_H
#define NETWORKLIBTYPE_H

class ConnID {
public:
	ConnID() {
		ip = 0x00000000;
		port = 0;
	}
	ConnID(unsigned long ip,unsigned short port){
        this->ip = ip;
        this->port = port;
	}
	ConnID& operator=(const ConnID& input) {
		ip = input.ip;
		port = input.port;
		return *this;
	}
	bool operator==(const ConnID& input) {
		return (ip == input.ip && port == input.port);
	}
    unsigned long ip;
	unsigned short port;
};
//连接建立确认包
class ConnAcceptPackage{
    unsigned char PackageId;
    ConnID connId;
    unsigned long sessionId;
}
//! 包的优先级
enum PackagePriority
{
	HIGH_PRIORITY,
	MEDIUM_PRIORITY,
	LOW_PRIORITY,
	NUMBER_OF_PRIORITIES
};
//! 包的属性
enum PackageReliability
{
	UNRELIABLE,
	UNRELIABLE_SEQUENCED,
	RELIABLE,
	RELIABLE_ORDERED,
	RELIABLE_SEQUENCED
};
#endif // !__NETWORKLIBTYPE_H
