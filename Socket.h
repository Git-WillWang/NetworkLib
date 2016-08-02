#pragma once
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<arpa/inet.h>
#include "NetworkLibType.h"
#define INVALD_SOCKET -1
class Socket {
public:
	Socket();
	~Socket();
	static inline Socket* getInstance(){return &s;}
	SOCKET createListenSocket(unsigned short port, bool useAnyPort = false, bool onlyLocal = false);
	SOCKET connect(SOCKET socket, ConnID& connId);
	int send(SOCKET s, char *data, int length, unsigned char ip[16], unsigned short port);
	int send(SOCKET s, char *data, int length, ConnID& ConnId);
    bool hostnameToIp(const char* hostname,char* Ip);
	bool getHostIp(char ipList[10][16]);
private:
    static Socket s;
};
