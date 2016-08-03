#include "PackagePool.h"


PackageManager::PackageManager() {
	while (!pool.empty()) pool.pop();
	for (int s = 0; s < DEFAULT_PACKAGE_NUM; ++s)
		pool.push(new Package());
}
InternalPackageManager::InternalPackageManager(int num) {
	if (num <= 0) num = DEFAULT_PACKAGE_NUM;
	while (!pool.empty()) pool.pop();
	for (int s = 0; s < num; ++s)
		pool.push(new Package());
}
void PackageManager::realease(Package *p){
    pool.push(p);
}
static Package* PackageManager::getInstance(){
    if(pool.empty()) return NULL;
    Package* p =pool.front();
    pool.pop();
    return p;
}
bool PackageManager::extend(int num){
    int size = pool.size();
    if (num <= 0 || size>=MAX_PACKAGE_NUNM)
		return false;
	if (size+ num >= MAX_PACKAGE_NUNM)
		num = MAX_PACKAGE_NUNM - size;
	for (int s = 0; s < num; ++s)
		pool.push(new Package());
	return true;
}
