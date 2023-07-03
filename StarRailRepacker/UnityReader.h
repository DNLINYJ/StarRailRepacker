#pragma once
#include <string>
#include <vector>
using namespace std;

struct StorageBlock {
	unsigned __int32 compressedSize = 0;
	unsigned __int32 uncompressedSize = 0;
	unsigned __int16 flags = 0;
};

struct Node
{
	unsigned __int64 offset = 0;
	unsigned __int64 size = 0;
	unsigned __int16 flags = 0;
	string path;
};

struct HeaderInfo {
	string signature;
	string unityVersion;
	string unityRevisions;
	unsigned int bundleSize;
	unsigned int compressedBlocksInfoSize;
	unsigned int uncompressedBlocksInfoSize;
	unsigned int flag;
};

struct BlocksAndDirectoryInfo {
	vector<StorageBlock> m_BlocksInfo;
	vector<Node> m_DirectoryInfo;
	unsigned int blocksInfoCount;
	unsigned int nodesCount;
};

std::string translate_hex_to_write_to_file(std::string hex);

string ReadStringToNull(fstream& bytes);

BlocksAndDirectoryInfo ReadBlocksInfoAndDirectory(unsigned char* uncompressedBlocksInfo);

HeaderInfo ReadHeader(fstream& normalUnityFile);