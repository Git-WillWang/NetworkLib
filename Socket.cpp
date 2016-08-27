#ifndef SOCKET_H
#define SOCKET_H
	Socket::Socket(){
	}
	Socket::~Socket(){
	}
	SOCKET Socket::createListenSocket(unsigned short port, bool useAnyPort = false, bool onlyLocal = false){
        SOCKET lsocket;
        sockaddr_in sa;
        lsocket = socket(onlyLocal?AF_UNIX:AF_INET,SOCK_DGRAM,0);
        if(lsocket == INVALD_SOCKET){
            return INVALD_SOCKET;
        }
        bool reuseAddr = true;
        if(!useAnyPort){
            if(setsockopt(lsocket,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseAddr,sizeof(reuseAddr))==-1){
                return -1;
            }
        }
        int result = 0;
        unsigned short i = 0;
        for(;;){
            sa.sin_port = htons(port+i);
            sa.sin_family = onlyLocal?AF_UNIX:AF_UNIF;
            sa.sin_addr.s_addr = INADDR_ANY;
            result = bind(lsocket,(socketaddr*)&sa,sizeof(sockaddr));
            if(result ==SOCKET_ERROR){
                if(useAnyPort){
                    ++i;
                    continue;
                }
                close(lsocket);
                return INVALD_SOCKET;
            }
            break;
        }
        return lsocket;
	}
	SOCKET Socket::connect(SOCKET socket, ConnID& connId){
        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = connId.ip;
        sa.sin_port = connId.port;
        if(connect(socket,(sockaddr*)&sa,sizeof(sockaddr))!=0){
            return -1;
        }
        return socket;
	}
	int Socket::send(SOCKET s, char *data, int length, ConnID& connId){
        if(s==INVALD_SOCKET){
            return -1;
        }
        sockaddr_in sa;
        sa.sin_addr = htons(connId.port);
        sa.sin_addr.s_addr = connId.ip;
        sa.sin_family = AF_INET;
        if(SOCKET_ERROR == sendto(s,data,length,0,(const sockaddr*)&sa,sizeof(sockaddr_in)))
            return 1;
        return 0;
	}
    int Socket::send(SOCKET s, char *data, int length, char ip[16], unsigned short port){
        return send(s,data,length,ConnID(inet_addr(ip),port));
	}
    bool Socket::hostnameToIp(const char* hostname,char* Ip){
        struct hostent *phe = gethostbyname(hostname);
        if(!phe || !phe->h_addr_list[0])
            return false;
        struct in_addr addr;
        memcpy(&addr,phe->h_addr_list[0],sizeof(in_addr));
        strcpy(Ip,inet_ntoa(addr));
        return true;
    }
	bool Socket::getHostIp(char* ip){
        char hostname[256];
        if(gethostname(hostname,256) == SOCKET_ERROR){
            return false;
        }
        struct hostent *phe = gethostbyname(hostname);
        if(!phe)
            return false;
        ip = inet_ntoa(*(in_addr*)phe->h_addr_list[0]);
        return true;
	}
#endif
