#include <iostream>
#include <Windows.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
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

int TranslateToMihoyoUnityFile(string inpath, string outpath, string saveformat) {

	cout << "[Debug] �����ļ�Ŀ¼:" << inpath << endl;
	cout << "[Debug] �����ļ�Ŀ¼:" << outpath << endl;

	fstream normalUnityFile;
	normalUnityFile.open(inpath, ios::in | ios::binary);

	// ReadHeader
	unsigned char version[sizeof(__int32)];
	unsigned char bundleSize[sizeof(__int64)];
	unsigned char compressedBlocksInfoSize[sizeof(__int32)];
	unsigned char uncompressedBlocksInfoSize[sizeof(__int32)];
	unsigned char flag[sizeof(__int32)];

	string signature = ReadStringToNull(normalUnityFile);

	normalUnityFile.clear();
	normalUnityFile.read(reinterpret_cast<char*>(version), sizeof(version));
	string unityVersion = ReadStringToNull(normalUnityFile);
	string unityRevisions = ReadStringToNull(normalUnityFile);
	normalUnityFile.read(reinterpret_cast<char*>(bundleSize), sizeof(bundleSize));
	normalUnityFile.read(reinterpret_cast<char*>(compressedBlocksInfoSize), sizeof(compressedBlocksInfoSize));
	normalUnityFile.read(reinterpret_cast<char*>(uncompressedBlocksInfoSize), sizeof(uncompressedBlocksInfoSize));
	normalUnityFile.read(reinterpret_cast<char*>(flag), sizeof(flag));

	// https://stackoverflow.com/questions/34780365/how-do-i-convert-an-unsigned-char-array-into-an-unsigned-long-long
	unsigned int bundleSize_unsigned_int;
	memcpy(&bundleSize_unsigned_int, bundleSize, sizeof bundleSize);
	bundleSize_unsigned_int = _byteswap_ulong(bundleSize_unsigned_int);

	unsigned int compressedBlocksInfoSize_unsigned_int = _byteswap_ulong(*(unsigned int*)compressedBlocksInfoSize);
	unsigned int uncompressedBlocksInfoSize_unsigned_int = _byteswap_ulong(*(unsigned int*)uncompressedBlocksInfoSize);
	unsigned int flag_unsigned_int = _byteswap_ulong(*(unsigned int*)flag);

	cout << "[Debug] compressedBlocksInfoSize: " << compressedBlocksInfoSize_unsigned_int << endl;
	cout << "[Debug] uncompressedBlocksInfoSize: " << uncompressedBlocksInfoSize_unsigned_int << endl;

	// ��ȡblocksInfoBytes
	std::byte* blocksInfoBytes = new std::byte[compressedBlocksInfoSize_unsigned_int];
	std::byte* uncompressedBlocksInfo;
	normalUnityFile.read(reinterpret_cast<char*>(blocksInfoBytes), compressedBlocksInfoSize_unsigned_int);

	// ��ȡblocksInfoBytes�ļ��ܷ�ʽ
	switch (flag_unsigned_int & 0x3F) {
	case 0x00:
		uncompressedBlocksInfo = blocksInfoBytes;
		break;
	case 0x02:
	case 0x03:
		uncompressedBlocksInfo = new std::byte[uncompressedBlocksInfoSize_unsigned_int];
		LZ4_decompress_safe((char*)blocksInfoBytes, (char*)uncompressedBlocksInfo, compressedBlocksInfoSize_unsigned_int, uncompressedBlocksInfoSize_unsigned_int);
		break;
	default:
		cout << "��֧�ֵ�ѹ������ [֧�ֵ�ѹ������ΪNone��LZ4]" << endl;
		return 0;
	}

	// ReadBlocksInfoAndDirectory

	// ����blockHash
	uncompressedBlocksInfo = uncompressedBlocksInfo + 16;
	unsigned int blocksInfoCount;
	memcpy(&blocksInfoCount, uncompressedBlocksInfo, 4i64);
	uncompressedBlocksInfo = uncompressedBlocksInfo + 4;
	blocksInfoCount = _byteswap_ulong(blocksInfoCount);

	cout << "[Debug] blocksInfoCount:" << blocksInfoCount << endl;

	StorageBlock* m_BlocksInfo = new StorageBlock[blocksInfoCount];

	for (int i = 0; i < blocksInfoCount; i++) {
		// ��С��ת��
		memcpy(&m_BlocksInfo[i].uncompressedSize, uncompressedBlocksInfo, 4i64);
		m_BlocksInfo[i].uncompressedSize = _byteswap_ulong(m_BlocksInfo[i].uncompressedSize);
		uncompressedBlocksInfo += 4i64;

		memcpy(&m_BlocksInfo[i].compressedSize, uncompressedBlocksInfo, 4i64);
		m_BlocksInfo[i].compressedSize = _byteswap_ulong(m_BlocksInfo[i].compressedSize);
		uncompressedBlocksInfo += 4i64;

		memcpy(&m_BlocksInfo[i].flags, uncompressedBlocksInfo, 2i64);
		m_BlocksInfo[i].flags = _byteswap_ushort(m_BlocksInfo[i].flags);
		uncompressedBlocksInfo += 2i64;
	}


	unsigned int nodesCount;
	if (blocksInfoCount) {
		memcpy(&nodesCount, uncompressedBlocksInfo, 4i64);
		uncompressedBlocksInfo += 4i64;
		nodesCount = _byteswap_ulong(nodesCount);
		cout << "[Debug] nodesCount:" << nodesCount << endl;
	}
	else
		return 0;

	Node* m_DirectoryInfo = new Node[nodesCount];

	unsigned int path_size = 0;
	char* v3 = new char;
	std::byte* uncompressedBlocksInfo_set;

	for (int i = 0; i < nodesCount; i++)
	{
		memcpy(&m_DirectoryInfo[i].offset, uncompressedBlocksInfo, 8i64);
		uncompressedBlocksInfo += 8i64;

		memcpy(&m_DirectoryInfo[i].size, uncompressedBlocksInfo, 8i64);
		uncompressedBlocksInfo += 8i64;

		memcpy(&m_DirectoryInfo[i].flags, uncompressedBlocksInfo, 4i64);
		m_DirectoryInfo[i].flags = _byteswap_ulong(m_DirectoryInfo[i].flags);
		uncompressedBlocksInfo += 4i64;

		uncompressedBlocksInfo_set = uncompressedBlocksInfo;

		// ReadStringToNull()
		for (int a = 0;; a++) {
			memcpy(v3, uncompressedBlocksInfo, 1i64);
			if ((int)*v3 == 0) {
				path_size++;
				uncompressedBlocksInfo++;
				break;
			}
			path_size++;
			uncompressedBlocksInfo++;
		}
		char* path_v1 = new char[path_size];
		strcpy_s(path_v1, path_size, (const char*)uncompressedBlocksInfo_set);
		m_DirectoryInfo[i].path = path_v1;
	}

	// CreateBlocksStream

	// Ϊ�����´����ΪcompressedSizeSum
	/*
	unsigned __int32 uncompressedSizeSum = 0;
	for (int i = 0; i < blocksInfoCount; i++) {
		uncompressedSizeSum += m_BlocksInfo[i].uncompressedSize;
	}

	std::byte* blocksStream = new std::byte[uncompressedSizeSum];
	*/

	// ����compressedSizeSum �����µ�block��ŵ��ڴ�
	// ��ȫ���� 0x14 �в������ܵĺ���������һ�¾���
	unsigned __int32 compressedSizeSum = 0;
	for (int i = 0; i < blocksInfoCount; i++) {
		compressedSizeSum += m_BlocksInfo[i].compressedSize + 0x14;
	}

	std::byte* blocksStream = new std::byte[compressedSizeSum];
	std::byte* blocksStream_start_address = blocksStream;

	// ReadBlocks(DecryptBlocks)

	std::byte* compressedBytes;
	std::byte* newCompressedBytes;

	for (int i = 0; i < blocksInfoCount; i++) {

		// ��ȡblock
		compressedBytes = new std::byte[m_BlocksInfo[i].compressedSize];
		normalUnityFile.read(reinterpret_cast<char*>(compressedBytes), m_BlocksInfo[i].compressedSize);

		// ����block
		if (m_BlocksInfo[i].compressedSize > 0xFF && (m_BlocksInfo[i].flags == 0x42 || m_BlocksInfo[i].flags == 0x43)) {

			newCompressedBytes = Encrypt(compressedBytes, m_BlocksInfo[i].compressedSize);

			// ��Ӽ��ܺ����Ĵ�С (mr0k�ļ�ͷ + key1 Ϊ 0x14 �ֽ�)
			m_BlocksInfo[i].compressedSize += 0x14;

			// �޸�Flag
			m_BlocksInfo[i].flags = 0x45; // Flag��ΪLZ4Mr0k

			memcpy(blocksStream, newCompressedBytes, m_BlocksInfo[i].compressedSize);
		}
		else {
			memcpy(blocksStream, compressedBytes, m_BlocksInfo[i].compressedSize);
		}

		blocksStream += m_BlocksInfo[i].compressedSize;

		//https://stackoverflow.com/questions/61443910/what-is-causing-invalid-address-specified-to-rtlvalidateheap-01480000-014a290
		//delete[] compressedBytes;
	}

	// ���¼���compressedSizeSum
	compressedSizeSum = 0;
	for (int i = 0; i < blocksInfoCount; i++) {
		compressedSizeSum += m_BlocksInfo[i].compressedSize;
	}

	// Output Unity Standard *.unity3d Files
	string ArchiveVersion = "00000006"; // 6 (4bit)

	// ��ȡ·����Ϣ�ܴ�С
	unsigned int nodesSize = 0;
	for (int i = 0; i < nodesCount; i++)
	{
		nodesSize = nodesSize + 20 + m_DirectoryInfo[i].path.length() + 1; // +1 ����ɱ��\00
	}

	fstream testoutput;
	testoutput.open(outpath, std::ios::binary | std::ios::out);

	string ENCR = "ENCR";

	// ��鱣����ʽ
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

	// ����µ�BlocksInfoBytes
	unsigned int NewBlocksInfoBytes_Size;
	std::byte* NewBlocksInfoBytes;
	std::byte* NewBlocksInfoBytes_Start_Address;

	// ���ݱ�����ʽд���Ӧ���ļ�ͷ
	unsigned __int64 ABPackSize;
	switch (std::stoi(saveformat)) {
	case 1: // ����
		testoutput << ENCR << '\x00';

		// 4 Ϊ����������4bit blockCount��
		// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
		// 4 Ϊ�ڵ����� (4bit nodeCount)
		// nodesSize Ϊ �ܽڵ��С
		NewBlocksInfoBytes_Size = 4 + (10 * blocksInfoCount) + 4 + nodesSize;
		NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
		NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

		// 20 Ϊ ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
		// 4 Ϊ������Ϣ��4bit blockCount��
		// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
		// nodesSize Ϊ ·����Ϣ�ܴ�С
		// compressedSizeSum Ϊ blocks �ܴ�С
		ABPackSize = 20 + 4 + (10 * blocksInfoCount) + nodesSize + compressedSizeSum;
		break;
	case 2:
	default: // ������
		testoutput << signature << '\x00';
		testoutput << translate_hex_to_write_to_file(ArchiveVersion);
		testoutput << unityVersion << '\x00';
		testoutput << unityRevisions << '\x00';

		// 20 Ϊ������Ϣ��16bit hash / 4bit blockCount��
		// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
		// 4 Ϊ�ڵ����� (4bit nodeCount)
		// nodesSize Ϊ �ܽڵ��С
		NewBlocksInfoBytes_Size = 20 + (10 * blocksInfoCount) + 4 + nodesSize;
		NewBlocksInfoBytes = new std::byte[NewBlocksInfoBytes_Size];
		NewBlocksInfoBytes_Start_Address = NewBlocksInfoBytes;

		// 20 Ϊ ABPackSize + compressedBlocksInfoSize + uncompressedBlocksInfoSize + flags
		// 20 Ϊ������Ϣ��16bit hash / 4bit blockCount��
		// 10 * blocksInfoCount Ϊ ��������Ϣ�ܴ�С
		// nodesSize Ϊ ·����Ϣ�ܴ�С
		// compressedSizeSum Ϊ blocks �ܴ�С
		ABPackSize = (signature.length() + 1) + 4 + (unityVersion.length() + 1) + (unityRevisions.length() + 1) + 20 + 20 + (10 * blocksInfoCount) + nodesSize + compressedSizeSum;

		// ����ԭblockHash ��00���
		unsigned __int64 blockHash = 0;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;
		memcpy(NewBlocksInfoBytes, &blockHash, 8i64);
		NewBlocksInfoBytes += 8i64;

		break;
	}

	// д��blocksInfoCount
	blocksInfoCount = _byteswap_ulong(blocksInfoCount);
	memcpy(NewBlocksInfoBytes, &blocksInfoCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// д��blocksInfo
	for (int i = 0; i < _byteswap_ulong(blocksInfoCount); i++) {
		// ��С��ת��
		m_BlocksInfo[i].uncompressedSize = _byteswap_ulong(m_BlocksInfo[i].uncompressedSize);
		memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].uncompressedSize, 4i64);
		NewBlocksInfoBytes += 4i64;

		m_BlocksInfo[i].compressedSize = _byteswap_ulong(m_BlocksInfo[i].compressedSize);
		memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].compressedSize, 4i64);
		NewBlocksInfoBytes += 4i64;

		m_BlocksInfo[i].flags = _byteswap_ushort(m_BlocksInfo[i].flags);
		memcpy(NewBlocksInfoBytes, &m_BlocksInfo[i].flags, 2i64);
		NewBlocksInfoBytes += 2i64;
	}

	// д��nodesCount
	nodesCount = _byteswap_ulong(nodesCount);
	memcpy(NewBlocksInfoBytes, &nodesCount, 4i64);
	NewBlocksInfoBytes += 4i64;

	// д��nodes
	for (int i = 0; i < _byteswap_ulong(nodesCount); i++)
	{
		// ����nodes��offset��sizeûת�������Ϊǰ�������תС���� ���üӴ����ˣ�O��
		memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].offset, 8i64);
		NewBlocksInfoBytes += 8i64;

		memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].size, 8i64);
		NewBlocksInfoBytes += 8i64;
		//m_DirectoryInfo[i].size = _byteswap_uint64(m_DirectoryInfo[i].size);

		m_DirectoryInfo[i].flags = _byteswap_ulong(m_DirectoryInfo[i].flags);
		memcpy(NewBlocksInfoBytes, &m_DirectoryInfo[i].flags, 4i64);
		NewBlocksInfoBytes += 4i64;

		char* path_v1 = new char[m_DirectoryInfo[i].path.length()];
		path_v1 = (char*)m_DirectoryInfo[i].path.c_str();
		memcpy(NewBlocksInfoBytes, path_v1, m_DirectoryInfo[i].path.length() + 1);// +1 ����ɱ��\00
		NewBlocksInfoBytes += m_DirectoryInfo[i].path.length() + 1;
	}

	// ���µ�BlocksInfoBytes��LZ4ѹ��
	unsigned int max_dst_size = LZ4_compressBound(NewBlocksInfoBytes_Size);
	std::byte* NewBlocksInfoBytes_LZ4 = new std::byte[max_dst_size];
	unsigned int NewBlocksInfoBytes_compSize = LZ4_compress_default((char*)NewBlocksInfoBytes_Start_Address, (char*)NewBlocksInfoBytes_LZ4, NewBlocksInfoBytes_Size, max_dst_size);

	// ���µ�BlocksInfoBytes����
	/*
	if (NewBlocksInfoBytes_compSize > 0xFF) {
		NewBlocksInfoBytes_LZ4 = Encrypt(NewBlocksInfoBytes_LZ4, NewBlocksInfoBytes_compSize);
		NewBlocksInfoBytes_compSize += 0x14;
		ArchiveFlags = "00000045";// LZ4Mr0kѹ��flags
	}*/

	// �ͷ��ڴ� ����ڴ汬ը
	delete[] NewBlocksInfoBytes_Start_Address;
	delete[] blocksInfoBytes;
	delete[] m_DirectoryInfo;
	delete[] m_BlocksInfo;

	// С��ת���
	ABPackSize = _byteswap_uint64(ABPackSize);
	testoutput.write((const char*)&ABPackSize, sizeof(unsigned __int64));

	NewBlocksInfoBytes_compSize = _byteswap_ulong(NewBlocksInfoBytes_compSize);
	testoutput.write((const char*)&NewBlocksInfoBytes_compSize, sizeof(unsigned int));

	NewBlocksInfoBytes_Size = _byteswap_ulong(NewBlocksInfoBytes_Size);
	testoutput.write((const char*)&NewBlocksInfoBytes_Size, sizeof(unsigned int));

	// д��Flag
	string ArchiveFlags = "00000043";// LZ4ѹ��flags
	testoutput << translate_hex_to_write_to_file(ArchiveFlags);

	// д��blocksInfoBytes
	testoutput.write((const char*)NewBlocksInfoBytes_LZ4, _byteswap_ulong(NewBlocksInfoBytes_compSize));

	// �ͷ��ڴ� ����ڴ汬ը
	delete[] NewBlocksInfoBytes_LZ4;

	// д��blocksStream
	testoutput.write((const char*)blocksStream_start_address, compressedSizeSum);

	// �ͷ��ڴ� ����ڴ汬ը
	delete[] blocksStream_start_address;

	return 0;
}