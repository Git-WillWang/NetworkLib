#include "BitStream.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>

unsigned char twoBitsMap[4][2] = { {0,0} ,{0,0x80},{0x80,0},{0x80,0x80} };
unsigned char threeBitsMap[8][3] = {{ 0,0,0 }, {0,0,0x80}, {0,0x80,0}, {0,0x80,0x80}, {0x80,0,0}, {0x80,0,0x80}, {0x80,0x80,0}, {0x80,0x80,0x80}};

int getType(const char* _data) {
	return 0;
}
int getType(const unsigned char* _data) {
	return 0;
}
int getType(const unsigned short* _data) {
	return 1;
}
int getType(const short* _data) {
	return 1;
}
int getType(const unsigned int* _data) {
	return 3;
}
int getType(const int* _data) {
	return 3;
}
int getType(const unsigned long* _data) {
	return 3;
}
int getType(const long* _data) {
	return 3;
}
int getType(const unsigned long long* _data) {
	return 7;
}
int getType(const long long* _data) {
	return 7;
}

BitStream::BitStream() {
	bitsUsed = 0;
	bitsAllocated = 256;
	readOffset = 0;
	data = (unsigned char*)malloc(32);
	memset(data, 0, 32);
}

BitStream::BitStream(int bytesToAlloc) {
	bitsUsed = 0;
	bitsAllocated = bytesToAlloc << 3;
	readOffset = 0;
	data = (unsigned char*)malloc(bitsAllocated);
	memset(data, 0, bytesToAlloc);
}

BitStream::BitStream(const unsigned char* _data, unsigned int length, bool isCopy) {
	bitsUsed = length << 3;
	bitsAllocated = length << 3;
	readOffset = 0;
	if (length > 0) {
		if (!isCopy) {
			data = const_cast<unsigned char*>(_data);
			return;
		}
		data = (unsigned char*)malloc(length);
		assert(data);
		memcpy(data, _data, length);
	}
	else {
		data = (unsigned char*)malloc(32);
		memset(data, 0, 32);
	}
}
BitStream::BitStream(const BitStream& copy) {
	bitsUsed = copy.bitsUsed;
	data = (unsigned char*)malloc(BITS_TO_BYTES(bitsUsed));
	memcpy(data, copy.data, BITS_TO_BYTES(bitsUsed));
	bitsAllocated = BITS_TO_BYTES(bitsUsed);
	readOffset = 0;
}
BitStream::~BitStream() {
	if (data) free(data);
}
template<class T>
void BitStream::write(const T* _data) {
	int s = getType(_data);
	if (reallocate((s + 1) * 8)) {
		if (!s) {
			writeBits((unsigned char*)_data, 8, true);
			return;
		}
		if (s == 1) {
			writeBits((unsigned char*)_data, 16, true);
			return;
		}
		unsigned char* pos = (unsigned char*)_data;
		//&优先级小于==
		unsigned char c = (pos[s] & 0x80) == 0x00 ? 0x00 : 0xFF;
		if (c == 0x00)
			writeBit(false);
		else
			writeBit(true);
		bool half = false;
		int t;
		for (t = s; t >= 0; --t) {
			if (pos[t] != c) {
				if ((c ^ 0xFF == 0 && (pos[t] & 0xF0) == 0xF0) || (c ^ 0x00 == 0 && (pos[t] & 0xF0) == 0x00))
					half = true;
				break;
			}
		}
		if (t < 0) {
			writeBit(true);
			return;
		}
		writeBit(false);
		t = s - t;
		if (s == 3) {
			data[bitsUsed >> 3] |= twoBitsMap[t][0] >> (bitsUsed % 8);
			++bitsUsed;
			data[bitsUsed >> 3] |= twoBitsMap[t][1] >> (bitsUsed % 8);
			++bitsUsed;
		}
		else {
			data[bitsUsed >> 3] |= threeBitsMap[t][0] >> (bitsUsed % 8);
			++bitsUsed;
			data[bitsUsed >> 3] |= threeBitsMap[t][1] >> (bitsUsed % 8);
			++bitsUsed;
			data[bitsUsed >> 3] |= threeBitsMap[t][2] >> (bitsUsed % 8);
			++bitsUsed;
		}
		writeBit(half);
		if (half)
			writeBits((unsigned char*)_data, (s - t) * 8 + 4, true);
		else
			writeBits((unsigned char*)_data, (s - t + 1) * 8, true);
	}
}
void BitStream::write(const bool& b) {
	if(reallocate(1)>=1)
		writeBit(b);
}
bool BitStream::read(bool& b) {
	return readBit(b);
}
void BitStream::writeBit(const bool& b) {
	if (b)
		data[bitsUsed >> 3] |= 0x80 >> (bitsUsed % 8);
	++bitsUsed;
}
bool BitStream::readBit(bool& b) {
	if (readOffset >= bitsUsed) return false;
	if (data[readOffset >> 3] & (0x80 >> (readOffset++ % 8)))
		b = true;
	else
		b = false;
	return true;
}
template<class T>
bool BitStream::read(T* output) {
	int s = getType(output);
	memset(output, 0, s + 1);
	if (!s)
		return readBits((unsigned char*)output, 8, true);
	if (s == 1)
		return readBits((unsigned char*)output, 16, true);
	bool checkBit;
	readBit(checkBit);
	unsigned char c = checkBit ? 0xFF : 0x00;
	unsigned char hc = checkBit ? 0xF0 : 0x00;
	readBit(checkBit);
	unsigned char* curPos = (unsigned char*)output;
	if (checkBit) {
		for (int i = 0; i <= s; ++i)
			curPos[i] |= c;
		return true;
	}
	int count = 0;
	int t = (s == 3) ? 2 : 3;
	for (int i = 0; i < t; ++i) {
		readBit(checkBit);
		count <<= 1;
		if (checkBit) count += 1;
	}
	for (int i = 0; i < count; ++i)
		curPos[s - i] |= c;
	readBit(checkBit);
	if (checkBit) {
		readBits((unsigned char*)output, (s - count) * 8 + 4, true);
		curPos[s - count] |= hc;
	}
	else {
		readBits((unsigned char*)output, (s - count + 1) * 8, true);
	}
	return true;
}
void BitStream::resetReadOffset() {
	readOffset = 0;
}
void BitStream::clear() {
	memset(data, 0, BITS_TO_BYTES(bitsUsed));
	bitsUsed = 0;
	readOffset = 0;
}
unsigned char* BitStream::getData(){
    return data;
}
unsigned int BitStream::getBytesUsed() {
	return BITS_TO_BYTES(bitsUsed);
}
unsigned int BitStream::getBitsUsed() {
	return bitsUsed;
}
unsigned int BitStream::getReadOffset() {
	return readOffset;
}
unsigned int BitStream::getBitsAllocated() {
	return bitsAllocated;
}
//重分配，使data有>=bitsLength的空间，返回空闲空间的大小
int BitStream::reallocate(int bitsLength) {
	if (bitsLength < 0) return -1;
	int bitsUnused = bitsAllocated - bitsUsed;
	if (bitsUnused < bitsLength) {
		int allocBits;
		if (bitsLength <= MIN_ALLOC_BITS)
			allocBits = MIN_ALLOC_BITS;
		else
			allocBits = bitsLength*2;
		int allocBytes = BITS_TO_BYTES(allocBits);
		//将分配空间初始化为0，否则会出错
		data = (unsigned char*)realloc(data, allocBytes+(bitsAllocated>>3));
		memset(data + (bitsAllocated >> 3), 0, allocBytes);
		bitsAllocated += allocBytes<<3;
		return bitsAllocated - bitsUsed;
	}
	else
		return bitsUnused;
}

void BitStream::writeBits(const unsigned char* _data, const int bitsToWrite,bool isRightAligned) {
	if (bitsToWrite <= 0) return;
	int b = bitsToWrite;
	int bitsUsedMod8 = bitsUsed % 8;
	int offset = 0;
	do {
		unsigned char c = _data[offset];
		if (b < 8 && isRightAligned)
			c <<= (8 - b);
		data[bitsUsed>>3] |= (c >> bitsUsedMod8);
		if (bitsUsedMod8 && b>(8 - bitsUsedMod8))
			data[(bitsUsed>>3)+1] |= (c << (8 - bitsUsedMod8));
		if (b >= 8)
			bitsUsed += 8;
		else
			bitsUsed += b;
		b -= 8;
		++offset;
	} while (b > 0);
}
bool BitStream::readBits(unsigned char* output, const int bitsToRead,bool isRightAligned) {
	if (bitsToRead <= 0||(readOffset + bitsToRead > bitsUsed))
		return false;
	unsigned char* curPos = output;
	int bitsLeft = bitsToRead;
	int readOffsetMod8 = readOffset % 8;
	do {
		unsigned char c = (data[readOffset >> 3] << readOffsetMod8);
		*curPos |= (data[readOffset>>3]<< readOffsetMod8);
		if (readOffsetMod8 && bitsLeft > 8 - readOffsetMod8)
			*curPos |= (data[(readOffset>>3)+1] >> (8 - readOffsetMod8));
		if (bitsLeft < 8) {
			*curPos >>= (8 - bitsLeft);
			if (!isRightAligned)
				*curPos <<= (8 - bitsLeft);
			readOffset += bitsLeft;
		}
		else
			readOffset += 8;
		++curPos;
		bitsLeft -= 8;
	} while (bitsLeft > 0);
	return true;
}

void BitStream::printBits() {
	if (bitsUsed <= 0) printf("No data to print\n");
	int bytesUsed = BITS_TO_BYTES(bitsUsed);
	int bitsOffset = (bitsUsed + 7) % 8;
	for (int i = 0; i < bytesUsed-1; ++i) {
		for (int s = 7; s >= 0; --s) {
			if (data[i] >> s & 1)
				putchar('1');
			else putchar('0');
		}
	}
	for (int i = 0; i <= bitsOffset; ++i) {
		if (data[bytesUsed - 1] & (0x80 >> i))
			putchar('1');
		else putchar('0');
	}
	putchar('\n');
}
template<class T>
void BitStream::print(const T* t) {
	int s = getType(t);
	unsigned char* pos = (unsigned char*)t;
	for (int i = s; i >= 0; --i) {
		for (int j = 0; j <= 7; ++j) {
			if (pos[i] & ((0x80) >> j))
				putchar('1');
			else putchar('0');
		}
	}
	putchar('\n');
}
void BitStream::reset(){
    memset(data,0,BITS_TO_BYTES(bitsUsed));
    readOffset = bitsUsed = 0;
}
