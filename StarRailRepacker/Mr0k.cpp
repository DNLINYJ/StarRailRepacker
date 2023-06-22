#include <iostream>
#include <Windows.h>
#include <fstream>
#include <string>
#include <random>
#include <cstdint>
using namespace std;

#include "AES-128.h"
#include "magic_constants.h"

uint8_t* GenKey3() {
	uint8_t* Key3 = new uint8_t[0x10];

	// 初始化随机数引擎
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dist(0, 255);

	// 使用随机数引擎填充数组
	for (size_t i = 0; i < 0x10; ++i) {
		Key3[i] = static_cast<uint8_t>(dist(gen));
	}

	return Key3;
}

void Decrypt(std::byte* bytes, unsigned int size) {
	uint8_t* key1 = new uint8_t[0x10];
	uint8_t* key2 = new uint8_t[0x10];
	uint8_t* key3 = new uint8_t[0x10];

	memcpy(key1, bytes + 4, 0x10);
	memcpy(key2, bytes + 0x74, 0x10);
	memcpy(key3, bytes + 0x84, 0x10);

	//__int32 encryptedBlockSize = std::min({ 0x10 * ((size - 0x94) >> 7), 0x400 });
	unsigned __int32 encryptedBlockSize;

	if (0x10 * ((size - 0x94) >> 7) < 0x400)
		encryptedBlockSize = 0x10 * ((size - 0x94) >> 7);
	else
		encryptedBlockSize = 0x400;

	std::byte* encryptedBlock = new std::byte[encryptedBlockSize];
	memcpy(encryptedBlock, bytes + 0x94, encryptedBlockSize);

	for (int i = 0; i < sizeof(ConstKey) / sizeof(*ConstKey); i++)
		key2[i] ^= ConstKey[i];

	AESDecrypt(key1, (uint8_t*)ExpansionKey);
	AESDecrypt(key3, (uint8_t*)ExpansionKey);

	for (int i = 0; i < 0x10; i++)
		key1[i] ^= key3[i];

	memcpy(bytes + 0x84, key1, 0x10);

	unsigned __int64 seed1;
	memcpy(&seed1, key2, sizeof key2);
	unsigned __int64 seed2;
	memcpy(&seed2, key3, sizeof key3);
	unsigned __int64 seed = seed2 ^ seed1 ^ (seed1 + (unsigned int)size - 20);

	std::byte seedBytes[sizeof(seed)];

	for (int i = 0; i < sizeof(seed); i++)
	{
		seedBytes[i] = (std::byte)((seed >> (8 * i)) & 0xFF);
	}

	for (int i = 0; i < encryptedBlockSize; i++)
	{
		encryptedBlock[i] ^= (std::byte)((uint8_t)seedBytes[i % 8] ^ Key[i]);
	}
	memcpy(bytes + 0x94, encryptedBlock, encryptedBlockSize);

	// 释放内存
	delete[] encryptedBlock;
}

// 用法 bytes为block指针 size为block的大小
std::byte* Encrypt(std::byte* bytes, unsigned int size) {
	uint8_t* key1 = new uint8_t[0x10];
	uint8_t* key2 = new uint8_t[0x10];
	uint8_t* key3 = GenKey3();

	unsigned int newSize = size + 0x14;

	memcpy(key2, bytes + 0x60, 0x10);
	memcpy(key1, bytes + 0x70, 0x10);

	for (int i = 0; i < sizeof(ConstKey) / sizeof(*ConstKey); i++)
		key2[i] ^= ConstKey[i];

	// 计算加密区块大小
	unsigned __int32 encryptBlockSize;
	if (0x10 * ((newSize - 0x94) >> 7) < 0x400)
		encryptBlockSize = 0x10 * ((newSize - 0x94) >> 7);
	else
		encryptBlockSize = 0x400;

	// 建立新的seedBytes
	unsigned __int64 seed1;
	memcpy(&seed1, key2, sizeof key2);
	unsigned __int64 seed2;
	memcpy(&seed2, key3, sizeof key3);
	unsigned __int64 seed = seed2 ^ seed1 ^ (seed1 + newSize - 20);

	std::byte seedBytes[sizeof(seed)];

	for (int i = 0; i < sizeof(seed); i++)
	{
		seedBytes[i] = (std::byte)((seed >> (8 * i)) & 0xFF);
	}

	// 加密区块
	std::byte* encryptBlock = new std::byte[encryptBlockSize];
	memcpy(encryptBlock, bytes + 0x80, encryptBlockSize);

	for (int i = 0; i < encryptBlockSize; i++)
	{
		encryptBlock[i] ^= (std::byte)((uint8_t)seedBytes[i % 8] ^ Key[i]);
	}

	for (int i = 0; i < 0x10; i++)
		key1[i] ^= key3[i];

	AESEncrypt(key1, (uint8_t*)ExpansionKey);
	AESEncrypt(key3, (uint8_t*)ExpansionKey);

	// 打包新的Block
	std::byte* newBlock = new std::byte[newSize];
	std::fill(newBlock, newBlock + newSize, std::byte{ 0 });
	newBlock[0] = static_cast<std::byte>('m');
	newBlock[1] = static_cast<std::byte>('r');
	newBlock[2] = static_cast<std::byte>('0');
	newBlock[3] = static_cast<std::byte>('k'); // 填充mr0k

	// 将key1复制到newBlock
	memcpy(newBlock + 4, key1, 0x10);

	// 将原block内容copy进新的内存
	memcpy(newBlock + 0x14, bytes, size);

	// 写入加密后的block
	memcpy(newBlock + 0x94, encryptBlock, encryptBlockSize);

	// 写入新的key3
	memcpy(newBlock + 0x84, key3, 0x10);

	// 释放内存
	delete[] encryptBlock;

	return newBlock;
}