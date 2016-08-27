#ifndef PACKAGEPOOL_H_INCLUDED
#define PACKAGEPOOL_H_INCLUDED
#include "NetworkLibType.h"
#include <queue>
#define MAX_PACKAGE_NUNM 1000
#define DEFAULT_PACKAGE_NUM 100
class Package {
	ConnID connID;
	unsigned long dataLength;
	unsigned char* data;
};

class PackageManager{
public:
    PackageManager();
    PackageManager(int size);
    ~PackageManager(){}
    void release(Package*,bool);
	static Package* getInstance();
	bool extend(int num);
private:
    queue<Package*> pool;
}
#endif // PACKAGEPOOL_H_INCLUDED
