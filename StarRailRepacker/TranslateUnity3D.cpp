#include <iostream>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "UnityReader.h"
#include "lz4.h"
#include "Mr0k.h"

int TranslateToMihoyoUnityFile(string inpath, string outpath) {

	cout << "[Debug] 待加密文件目录:" << inpath << endl;
	cout << "[Debug] 加密后文件目录:" << outpath << endl;

	fstream normalUnityFile;
	normalUnityFile.open(inpath, ios::in | ios::binary);

	if (!normalUnityFile.is_open()) {
		cout << "[Error] 打开源文件失败! " << endl;
		return 0;
	}

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

	// 打包新的BlocksInfoBytes
	unsigned int NewBlocksInfoBytes_Size;
	std::byte* NewBlocksInfoBytes;
	std::byte* NewBlocksInfoBytes_Start_Address;

	// 根据保存形式写入对应的文件头
	unsigned __int64 ABPackSize;
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