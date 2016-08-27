#pragma once
#ifndef __BITSTREAM_H
#define __BITSTREAM_H

#define MIN_ALLOC_BITS 8
#define BITS_TO_BYTES(x) ((x+7)>>3)
class BitStream {
public:
	BitStream();
	BitStream(int);
	BitStream(const unsigned char* data, unsigned int length,bool isCopy);
	BitStream(const BitStream& copy);
	virtual ~BitStream();

	void write(const bool&);
	bool read(bool&);
	template<class T>
	void write(const T*);
	template<class T>
	bool read(T*);
	void printBits();
	template<class>
	void print(const T*);
	void resetReadOffset();
	void clear();
	unsigned int getBitsUsed();
	unsigned int getBytesUsed();
	unsigned int getReadOffset();
	unsigned int getBitsAllocated();
	unsigned char* getData();
	bool readBits(unsigned char*, const int, bool);
	void writeBits(const unsigned char*, int, bool);
	void reset();

private:


	void writeBit(const bool&);
	bool readBit(bool&);
	int reallocate(int);
	int bitsUsed;
	int bitsAllocated;
	int readOffset;
	unsigned char* data;
};
#endif // !__BITSTREAM_H
