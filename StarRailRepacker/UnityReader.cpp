#include <iostream>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "UnityReader.h"

// From : https://stackoverflow.com/questions/21819782/writing-hex-to-a-file
std::string translate_hex_to_write_to_file(std::string hex) {
	std::basic_string<uint8_t> bytes;

	// Iterate over every pair of hex values in the input string (e.g. "18", "0f", ...)
	for (size_t i = 0; i < hex.length(); i += 2)
	{
		uint16_t byte;

		// Get current pair and store in nextbyte
		std::string nextbyte = hex.substr(i, 2);

		// Put the pair into an istringstream and stream it through std::hex for
		// conversion into an integer value.
		// This will calculate the byte value of your string-represented hex value.
		std::istringstream(nextbyte) >> std::hex >> byte;

		// As the stream above does not work with uint8 directly,
		// we have to cast it now.
		// As every pair can have a maximum value of "ff",
		// which is "11111111" (8 bits), we will not lose any information during this cast.
		// This line adds the current byte value to our final byte "array".
		bytes.push_back(static_cast<uint8_t>(byte));
	}

	// we are now generating a string obj from our bytes-"array"
	// this string object contains the non-human-readable binary byte values
	// therefore, simply reading it would yield a String like ".0n..:j..}p...?*8...3..x"
	// however, this is very useful to output it directly into a binary file like shown below
	std::string result(begin(bytes), end(bytes));

	return result;
}

string ReadStringToNull(fstream& bytes) {
	//std::byte* m = bytes;
	unsigned int start_ptr = bytes.tellg();
	char* v3 = new char;
	int path_size = 0;
	for (int a = 0;; a++) {
		bytes.read(v3, 1);
		if ((int)*v3 == 0) {
			path_size++;
			break;
		}
		path_size++;
	}
	char* path_v1 = new char[path_size];
	bytes.clear();
	bytes.seekg(start_ptr);
	bytes.read(path_v1, path_size);
	string path = path_v1;
	return path;
}

// ReadBlocksInfoAndDirectory º¯Êý
BlocksAndDirectoryInfo ReadBlocksInfoAndDirectory(unsigned char* uncompressedBlocksInfo) {
	BlocksAndDirectoryInfo result;

	// Ìø¹ýblockHash
	uncompressedBlocksInfo += 16;
	unsigned int blocksInfoCount;
	memcpy(&blocksInfoCount, uncompressedBlocksInfo, 4);
	uncompressedBlocksInfo += 4;
	blocksInfoCount = _byteswap_ulong(blocksInfoCount);

	result.blocksInfoCount = blocksInfoCount;

	cout << "[Debug] blocksInfoCount:" << blocksInfoCount << endl;

	for (int i = 0; i < blocksInfoCount; i++) {
		StorageBlock block;

		memcpy(&block.uncompressedSize, uncompressedBlocksInfo, 4);
		block.uncompressedSize = _byteswap_ulong(block.uncompressedSize);
		uncompressedBlocksInfo += 4;

		memcpy(&block.compressedSize, uncompressedBlocksInfo, 4);
		block.compressedSize = _byteswap_ulong(block.compressedSize);
		uncompressedBlocksInfo += 4;

		memcpy(&block.flags, uncompressedBlocksInfo, 2);
		block.flags = _byteswap_ushort(block.flags);
		uncompressedBlocksInfo += 2;

		result.m_BlocksInfo.push_back(block);
	}

	unsigned int nodesCount;
	if (blocksInfoCount) {
		memcpy(&nodesCount, uncompressedBlocksInfo, 4);
		uncompressedBlocksInfo += 4;
		nodesCount = _byteswap_ulong(nodesCount);
		cout << "[Debug] nodesCount:" << nodesCount << endl;

		result.nodesCount = nodesCount;
	}
	else {
		return result;
	}

	for (int i = 0; i < nodesCount; i++) {
		Node node;

		memcpy(&node.offset, uncompressedBlocksInfo, 8);
		uncompressedBlocksInfo += 8;

		memcpy(&node.size, uncompressedBlocksInfo, 8);
		uncompressedBlocksInfo += 8;

		memcpy(&node.flags, uncompressedBlocksInfo, 4);
		node.flags = _byteswap_ulong(node.flags);
		uncompressedBlocksInfo += 4;

		string path;
		for (;;) {
			char c;
			memcpy(&c, uncompressedBlocksInfo, 1);
			if (c == '\0') {
				uncompressedBlocksInfo++;
				break;
			}
			path.push_back(c);
			uncompressedBlocksInfo++;
		}
		node.path = path;

		result.m_DirectoryInfo.push_back(node);
	}

	return result;
}

HeaderInfo ReadHeader(fstream& normalUnityFile) {
	HeaderInfo headerInfo;

	unsigned char version[sizeof(__int32)];
	unsigned char bundleSize[sizeof(__int64)];
	unsigned char compressedBlocksInfoSize[sizeof(__int32)];
	unsigned char uncompressedBlocksInfoSize[sizeof(__int32)];
	unsigned char flag[sizeof(__int32)];

	headerInfo.signature = ReadStringToNull(normalUnityFile);
	normalUnityFile.clear();
	normalUnityFile.read(reinterpret_cast<char*>(version), sizeof(version));
	headerInfo.unityVersion = ReadStringToNull(normalUnityFile);
	headerInfo.unityRevisions = ReadStringToNull(normalUnityFile);
	normalUnityFile.read(reinterpret_cast<char*>(bundleSize), sizeof(bundleSize));
	normalUnityFile.read(reinterpret_cast<char*>(compressedBlocksInfoSize), sizeof(compressedBlocksInfoSize));
	normalUnityFile.read(reinterpret_cast<char*>(uncompressedBlocksInfoSize), sizeof(uncompressedBlocksInfoSize));
	normalUnityFile.read(reinterpret_cast<char*>(flag), sizeof(flag));

	memcpy(&headerInfo.bundleSize, bundleSize, sizeof bundleSize);
	headerInfo.bundleSize = _byteswap_ulong(headerInfo.bundleSize);

	headerInfo.compressedBlocksInfoSize = _byteswap_ulong(*(unsigned int*)compressedBlocksInfoSize);
	headerInfo.uncompressedBlocksInfoSize = _byteswap_ulong(*(unsigned int*)uncompressedBlocksInfoSize);
	headerInfo.flag = _byteswap_ulong(*(unsigned int*)flag);

	return headerInfo;
}