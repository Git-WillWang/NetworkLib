#include"InternalPackageManager.h"

InternalPackageManager::InternalPackageManager() {
	while (!pool.empty()) pool.pop();
	for (int s = 0; s < DEFAULT_INTERNAL_PACKAGE_NUM; ++s)
		pool.push(new InternalPackage());
	size = DEFAULT_INTERNAL_PACKAGE_NUM;
}
InternalPackageManager::InternalPackageManager(int num) {
	if (num <= 0) num = DEFAULT_INTERNAL_PACKAGE_NUM;
	while (!pool.empty()) pool.pop();
	for (int s = 0; s < num; ++s)
		pool.push(new InternalPackage());
	size = num;
}

InternalPackageManager::~InternalPackageManager() {
	while (!pool.empty()) {
		InternalPackage* p = pool.front();
		pool.pop();
		delete p;
	}
}

void InternalPackageManager::realease(InternalPackage* package,bool canFree) {
	if (package) {
		if (canFree)
			delete[] package;
		pool.push(package);
	}
}

InternalPackage* InternalPackageManager::getInstance() {
	if (pool.empty()) return NULL;
	InternalPackage* p = pool.front();
	pool.pop();
	return p;
}

bool InternalPackageManager::extend(int num) {
	if (num <= 0 || size>=MAX_INTERNAL_PACKAGE_NUM)
		return false;
	if (size + num >= MAX_INTERNAL_PACKAGE_NUM)
		num = MAX_INTERNAL_PACKAGE_NUM - size;
	for (int s = 0; s < num; ++s)
		pool.push(new InternalPackage());
	return true;
}
