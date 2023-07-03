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

	cout << "[Debug] �������ļ�Ŀ¼:" << inpath << endl;
	cout << "[Debug] ���ܺ��ļ�Ŀ¼:" << outpath << endl;

	fstream normalUnityFile;
	normalUnityFile.open(inpath, ios::in | ios::binary);

	if (!normalUnityFile.is_open()) {
		cout << "[Error] ��Դ�ļ�ʧ��! " << endl;
		return 0;
	}

	// ReadHeader
	HeaderInfo m_Header = ReadHeader(normalUnityFile);

	cout << "[Debug] compressedBlocksInfoSize: " << m_Header.compressedBlocksInfoSize << endl;
	cout << "[Debug] uncompressedBlocksInfoSize: " << m_Header.uncompressedBlocksInfoSize << endl;

	// ��ȡblocksInfoBytes
	std::byte* blocksInfoBytes = new std::byte[m_Header.compressedBlocksInfoSize];
	std::byte* uncompressedBlocksInfo;
	normalUnityFile.read(reinterpret_cast<char*>(blocksInfoBytes), m_Header.compressedBlocksInfoSize);

	// ��ȡblocksInfoBytes�ļ��ܷ�ʽ
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
		cout << "��֧�ֵ�ѹ������ [֧�ֵ�ѹ������ΪNone��LZ4]" << endl;
		return 0;
	}

	// ReadBlocksInfoAndDirectory
	BlocksAndDirectoryInfo blocksAndDirectoryInfo = ReadBlocksInfoAndDirectory((unsigned char*)uncompressedBlocksInfo);

	// CreateBlocksStream
	// ����compressedSizeSum �����µ�block��ŵ��ڴ�
	// ��ȫ���� 0x14 �в������ܵĺ���������һ�¾���
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
		// ��ȡblock
		compressedBytes = new std::byte[block.compressedSize];
		normalUnityFile.read(reinterpret_cast<char*>(compressedBytes), block.compressedSize);

		// ����block
		if (block.compressedSize > 0xFF && (block.flags == 0x42 || block.flags == 0x43)) {
			newCompressedBytes = Encrypt(compressedBytes, block.compressedSize);
			// ���Ӽ��ܺ����Ĵ�С (mr0k�ļ�ͷ + key1 Ϊ 0x14 �ֽ�)
			block.compressedSize += 0x14;
			// �޸�Flag
			block.flags = 0x45; // Flag��ΪLZ4Mr0k
			memcpy(blocksStream, newCompressedBytes, block.compressedSize);
		}
		else {
			memcpy(blocksStream, compressedBytes, block.compressedSize);
		}

		blocksStream += block.compressedSize;
	}

	// ���¼���compressedSizeSum
	compressedSizeSum = 0;
	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		compressedSizeSum += block.compressedSize;
	}

	// Output Unity Standard *.unity3d Files
	string ArchiveVersion = "00000006"; // 6 (4bit)

	// ��ȡ·����Ϣ�ܴ�С
	unsigned int nodesSize = 0;
	for (auto& node : blocksAndDirectoryInfo.m_DirectoryInfo)
	{
		nodesSize = nodesSize + 20 + node.path.length() + 1; // +1 ����ɱ��\00
	}

	fstream testoutput;
	testoutput.open(outpath, std::ios::binary | std::ios::out);

	string ENCR = "ENCR";

	// ����µ�BlocksInfoBytes
	unsigned int NewBlocksInfoBytes_Size;
	std::byte* NewBlocksInfoBytes;
	std::byte* NewBlocksInfoBytes_Start_Address;

	// ���ݱ�����ʽд���Ӧ���ļ�ͷ
	unsigned __int64 ABPackSize;
	testoutput << ENCR << '\x00';

	// 4 Ϊ����������4bit blockCount��
	// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
	// 4 Ϊ�ڵ����� (4bit nodeCount)
	// nodesSize Ϊ �ܽڵ��С
	NewBlocksInfoBytes_Size = 4 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + 4 + nodesSize;
	NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
	NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

	// 20 Ϊ ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
	// 4 Ϊ������Ϣ��4bit blockCount��
	// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
	// nodesSize Ϊ ·����Ϣ�ܴ�С
	// compressedSizeSum Ϊ blocks �ܴ�С
	ABPackSize = 20 + 4 + (10 * blocksAndDirectoryInfo.blocksInfoCount) + nodesSize + compressedSizeSum;

	// д��blocksInfoCount
	blocksAndDirectoryInfo.blocksInfoCount = _byteswap_ulong(blocksAndDirectoryInfo.blocksInfoCount);
	memcpy(NewBlocksInfoBytes, &blocksAndDirectoryInfo.blocksInfoCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// д��blocksInfo
	for (auto& block : blocksAndDirectoryInfo.m_BlocksInfo) {
		// ��С��ת��
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

	// д��nodesCount
	blocksAndDirectoryInfo.nodesCount = _byteswap_ulong(blocksAndDirectoryInfo.nodesCount);
	memcpy(NewBlocksInfoBytes, &blocksAndDirectoryInfo.nodesCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// д��nodes
	for (auto& node : blocksAndDirectoryInfo.m_DirectoryInfo)
	{
		// ����nodes��offset��sizeûת�������Ϊǰ�������תС���� ���üӴ����ˣ�O��
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
		memcpy(NewBlocksInfoBytes, path_v1, node.path.length() + 1);// +1 ����ɱ��\00
		NewBlocksInfoBytes += node.path.length() + 1;
	}

	// ���µ�BlocksInfoBytes��LZ4ѹ��
	unsigned int max_dst_size = LZ4_compressBound(NewBlocksInfoBytes_Size);
	std::byte* NewBlocksInfoBytes_LZ4 = new std::byte[max_dst_size];
	unsigned int NewBlocksInfoBytes_compSize = LZ4_compress_default((char*)NewBlocksInfoBytes_Start_Address, (char*)NewBlocksInfoBytes_LZ4, NewBlocksInfoBytes_Size, max_dst_size);

	// С��ת���
	ABPackSize = _byteswap_uint64(ABPackSize);
	testoutput.write((const char*)&ABPackSize, sizeof(unsigned __int64));

	NewBlocksInfoBytes_compSize = _byteswap_ulong(NewBlocksInfoBytes_compSize);
	testoutput.write((const char*)&NewBlocksInfoBytes_compSize, sizeof(unsigned int));

	NewBlocksInfoBytes_Size = _byteswap_ulong(NewBlocksInfoBytes_Size);
	testoutput.write((const char*)&NewBlocksInfoBytes_Size, sizeof(unsigned int));

	cout << "[Debug] NewBlocksInfoBytes_compSize: " << _byteswap_ulong(NewBlocksInfoBytes_compSize) << endl;
	cout << "[Debug] NewBlocksInfoBytes_Size: " << _byteswap_ulong(NewBlocksInfoBytes_Size) << endl;

	// д��Flag
	string ArchiveFlags = "00000043";// LZ4ѹ��flags
	testoutput << translate_hex_to_write_to_file(ArchiveFlags);

	// д��blocksInfoBytes
	testoutput.write((const char*)NewBlocksInfoBytes_LZ4, _byteswap_ulong(NewBlocksInfoBytes_compSize));

	// д��blocksStream
	testoutput.write((const char*)blocksStream_start_address, compressedSizeSum);

	return 0;
}