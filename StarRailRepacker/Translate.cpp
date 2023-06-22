#include <iostream>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "lz4.h"
#include "Mr0k.h"

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

// ReadBlocksInfoAndDirectory 函数
BlocksAndDirectoryInfo ReadBlocksInfoAndDirectory(unsigned char* uncompressedBlocksInfo) {
	BlocksAndDirectoryInfo result;

	// 跳过blockHash
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

int TranslateToMihoyoUnityFile(string inpath, string outpath, string saveformat) {

	cout << "[Debug] 加密文件目录:" << inpath << endl;
	cout << "[Debug] 解密文件目录:" << outpath << endl;

	fstream normalUnityFile;
	normalUnityFile.open(inpath, ios::in | ios::binary);

	// ReadHeader
	HeaderInfo m_Header = ReadHeader(normalUnityFile);

	cout << "[Debug] compressedBlocksInfoSize: " << m_Header.compressedBlocksInfoSize << endl;
	cout << "[Debug] uncompressedBlocksInfoSize: " << m_Header.uncompressedBlocksInfoSize << endl;

	// 读取blocksInfoBytes
	std::byte* blocksInfoBytes = new std::byte[m_Header.compressedBlocksInfoSize];
	std::byte* uncompressedBlocksInfo;
	normalUnityFile.read(reinterpret_cast<char*>(blocksInfoBytes), m_Header.compressedBlocksInfoSize);

	// 获取blocksInfoBytes的加密方式
	switch (m_Header.flag & 0x3F) {
	case 0x00:
		uncompressedBlocksInfo = blocksInfoBytes;
		break;
	case 0x02:
	case 0x03:
		uncompressedBlocksInfo = new std::byte[m_Header.uncompressedBlocksInfoSize];
		LZ4_decompress_safe((char*)blocksInfoBytes, (char*)uncompressedBlocksInfo, m_Header.compressedBlocksInfoSize, m_Header.uncompressedBlocksInfoSize);
		break;
	default:
		cout << "不支持的压缩类型 [支持的压缩类型为None和LZ4]" << endl;
		return 0;
	}

	// ReadBlocksInfoAndDirectory
	BlocksAndDirectoryInfo blocksAndDirectoryInfo = ReadBlocksInfoAndDirectory((unsigned char*)uncompressedBlocksInfo);

	// CreateBlocksStream
	// 计算compressedSizeSum 分配新的block存放的内存
	// 先全部加 0x14 有不被加密的后面重新算一下就行
	unsigned __int32 compressedSizeSum = 0;
	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		compressedSizeSum += block.compressedSize + 0x14;
	}

	std::byte* blocksStream = new std::byte[compressedSizeSum];
	std::byte* blocksStream_start_address = blocksStream;

	// ReadBlocks(DecryptBlocks)

	std::byte* compressedBytes;
	std::byte* newCompressedBytes;

	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		// 读取block
		compressedBytes = new std::byte[block.compressedSize];
		normalUnityFile.read(reinterpret_cast<char*>(compressedBytes), block.compressedSize);

		// 加密block
		if (block.compressedSize > 0xFF && (block.flags == 0x42 || block.flags == 0x43)) {

			newCompressedBytes = Encrypt(compressedBytes, block.compressedSize);

			// 添加加密后多出的大小 (mr0k文件头 + key1 为 0x14 字节)
			block.compressedSize += 0x14;

			// 修改Flag
			block.flags = 0x45; // Flag改为LZ4Mr0k

			memcpy(blocksStream, newCompressedBytes, block.compressedSize);
		}
		else {
			memcpy(blocksStream, compressedBytes, block.compressedSize);
		}

		blocksStream += block.compressedSize;
	}

	// 重新计算compressedSizeSum
	compressedSizeSum = 0;
	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		compressedSizeSum += block.compressedSize;
	}

	// Output Unity Standard *.unity3d Files
	string ArchiveVersion = "00000006"; // 6 (4bit)

	// 获取路径信息总大小
	unsigned int nodesSize = 0;
	for (auto& node : blocksAndDirectoryInfo.m_DirectoryInfo)
	{
		nodesSize = nodesSize + 20 + node.path.length() + 1; // +1 是天杀的\00
	}

	fstream testoutput;
	testoutput.open(outpath, std::ios::binary | std::ios::out);

	string ENCR = "ENCR";

	// 检查保存形式
	if (saveformat.empty()) {
		saveformat = "1";
	}
	else {
		try {
			// Attempt to convert the string to an integer
			int saveformatInt = std::stoi(saveformat);
		}
		catch (const std::invalid_argument& e) {
			saveformat = "1";
		}
		catch (const std::out_of_range& e) {
			saveformat = "1";
		}
	}

	// 打包新的BlocksInfoBytes
	unsigned int NewBlocksInfoBytes_Size;
	std::byte* NewBlocksInfoBytes;
	std::byte* NewBlocksInfoBytes_Start_Address;

	// 根据保存形式写入对应的文件头
	unsigned __int64 ABPackSize;
	switch (std::stoi(saveformat)) {
	case 1: // 星铁
		testoutput << ENCR << '\x00';

		// 4 为区块数量（4bit blockCount）
		// 10 * blocksInfoCount 为 各区块信息总大小
		// 4 为节点数量 (4bit nodeCount)
		// nodesSize 为 总节点大小
		NewBlocksInfoBytes_Size = 4 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + 4 + nodesSize;
		NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
		NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

		// 20 为 ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
		// 4 为区块信息（4bit blockCount）
		// 10 * blocksInfoCount 为 各区块信息总大小
		// nodesSize 为 路径信息总大小
		// compressedSizeSum 为 blocks 总大小
		ABPackSize = 20 + 4 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + nodesSize + compressedSizeSum;
		break;
	case 2:
	default: // 绝区零
		testoutput << m_Header.signature << '\x00';
		testoutput << translate_hex_to_write_to_file(ArchiveVersion);
		testoutput << m_Header.unityVersion << '\x00';
		testoutput << m_Header.unityRevisions << '\x00';

		// 20 为区块信息（16bit hash / 4bit blockCount）
		// 10 * blocksInfoCount 为 各区块信息总大小
		// 4 为节点数量 (4bit nodeCount)
		// nodesSize 为 总节点大小
		NewBlocksInfoBytes_Size = 20 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + 4 + nodesSize;
		NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
		NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

		// 20 为 ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
		// 20 为区块信息（16bit hash / 4bit blockCount）
		// 10 * blocksInfoCount 为 各区块信息总大小
		// nodesSize 为 路径信息总大小
		// compressedSizeSum 为 blocks 总大小
		ABPackSize = (m_Header.signature.length() + 1) + 4 + (m_Header.unityVersion.length() + 1) + (m_Header.unityRevisions.length() + 1) + 20 + 20 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + nodesSize + compressedSizeSum;

		// 不复原blockHash 用00填充
		unsigned __int64 blockHash = 0;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;

		break;
	}

	// 写入blocksInfoCount
	blocksAndDirectoryInfo.blocksInfoCount = _byteswap_ulong(blocksAndDirectoryInfo.blocksInfoCount);
	memcpy(NewBlocksInfoBytes, &blocksAndDirectoryInfo.blocksInfoCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// 写入blocksInfo
	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		// 大小端转换
		block.uncompressedSize = _byteswap_ulong(block.uncompressedSize);
		memcpy(NewBlocksInfoBytes, &block.uncompressedSize, 4i64);
		NewBlocksInfoBytes += 4i64;

		block.compressedSize = _byteswap_ulong(block.compressedSize);
		memcpy(NewBlocksInfoBytes, &block.compressedSize, 4i64);
		NewBlocksInfoBytes += 4i64;

		block.flags = _byteswap_ushort(block.flags);
		memcpy(NewBlocksInfoBytes, &block.flags, 2i64);
		NewBlocksInfoBytes += 2i64;
	}

	// 写入nodesCount
	blocksAndDirectoryInfo.nodesCount = _byteswap_ulong(blocksAndDirectoryInfo.nodesCount);
	memcpy(NewBlocksInfoBytes, &blocksAndDirectoryInfo.nodesCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// 写入nodes
	for (auto& node : blocksAndDirectoryInfo.m_DirectoryInfo)
	{
		// 这里nodes的offset和size没转大端是因为前面就忘记转小端了 懒得加代码了－O－
		memcpy(NewBlocksInfoBytes, &node.offset, 8i64);
		NewBlocksInfoBytes += 8i64;

		memcpy(NewBlocksInfoBytes, &node.size, 8i64);
		NewBlocksInfoBytes += 8i64;
		//node.size = _byteswap_uint64(node.size);

		node.flags = _byteswap_ulong(node.flags);
		memcpy(NewBlocksInfoBytes, &node.flags, 4i64);
		NewBlocksInfoBytes += 4i64;

		char* path_v1 = new char[node.path.length()];
		path_v1 = (char*)node.path.c_str();
		memcpy(NewBlocksInfoBytes, path_v1, node.path.length() + 1);// +1 是天杀的\00
		NewBlocksInfoBytes += node.path.length() + 1;
	}

	// 将新的BlocksInfoBytes用LZ4压缩
	unsigned int max_dst_size = LZ4_compressBound(NewBlocksInfoBytes_Size);
	std::byte* NewBlocksInfoBytes_LZ4 = new std::byte[max_dst_size];
	unsigned int NewBlocksInfoBytes_compSize = LZ4_compress_default((char*)NewBlocksInfoBytes_Start_Address, (char*)NewBlocksInfoBytes_LZ4, NewBlocksInfoBytes_Size, max_dst_size);

	// 小端转大端
	ABPackSize = _byteswap_uint64(ABPackSize);
	testoutput.write((const char*)&ABPackSize, sizeof(unsigned __int64));

	NewBlocksInfoBytes_compSize = _byteswap_ulong(NewBlocksInfoBytes_compSize);
	testoutput.write((const char*)&NewBlocksInfoBytes_compSize, sizeof(unsigned int));

	NewBlocksInfoBytes_Size = _byteswap_ulong(NewBlocksInfoBytes_Size);
	testoutput.write((const char*)&NewBlocksInfoBytes_Size, sizeof(unsigned int));

	cout << "[Debug] NewBlocksInfoBytes_compSize: " << _byteswap_ulong(NewBlocksInfoBytes_compSize) << endl;
	cout << "[Debug] NewBlocksInfoBytes_Size: " << _byteswap_ulong(NewBlocksInfoBytes_Size) << endl;

	// 写入Flag
	string ArchiveFlags = "00000043";// LZ4压缩flags
	testoutput << translate_hex_to_write_to_file(ArchiveFlags);

	// 写入blocksInfoBytes
	testoutput.write((const char*)NewBlocksInfoBytes_LZ4, _byteswap_ulong(NewBlocksInfoBytes_compSize));

	// 写入blocksStream
	testoutput.write((const char*)blocksStream_start_address, compressedSizeSum);

	return 0;
}