#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "Translate.h"

void main(int argc, char** argv) {
	if (argc == 1) {
		cout << "������Unity3d�ļ�תΪMr0k Unity3d�ļ� By:����С����" << endl;
		cout << "������ϷΪ: ����:�������һ���� ������һ��" << endl;
		cout << "�����������Ⱥ��������ֹ�⴫������" << endl;
		cout << endl;
		cout << "����: <inpath> <outpath> <savefromat>" << endl;
		cout << "inpath : �������ļ�·��" << endl;
		cout << "outpath : ���ܺ��ļ�·��" << endl;
		cout << "savefromat : 1 Ϊ���������ʽ 2 Ϊ�������ʽ [Ĭ���������������ʽ]" << endl;
	}
	else {
		string args[4];
		for (int i = 0; i < argc; i++) {
			args[i] = argv[i];
		}
		if (!args[1].empty() && !args[2].empty()) {
			TranslateToMihoyoUnityFile(args[1], args[2], args[3]);
		}
		else {
			cout << "������Unity3d�ļ�תΪMr0k Unity3d�ļ� By:����С����" << endl;
			cout << "������ϷΪ: ����:�������һ���� ������һ��" << endl;
			cout << "�����������Ⱥ��������ֹ�⴫������" << endl;
			cout << endl;
			cout << "����: <inpath> <outpath> <savefromat>" << endl;
			cout << "inpath : �������ļ�·��" << endl;
			cout << "outpath : ���ܺ��ļ�·��" << endl;
			cout << "savefromat : 1 Ϊ���������ʽ 2 Ϊ�������ʽ [Ĭ���������������ʽ]" << endl;
		}
	}
}