#include <iostream>
#include <filesystem>
#include <Windows.h>
namespace fs = std::filesystem;

#include "TranslateUnity3D.h"
#include "TranslateBlock.h"

void PrintNotice();

void main(int argc, char** argv) {
	SetConsoleTitle(L"StarRailRepacker By:DNLINYJ Version: 1.0.0");
	if (argc == 1) {
		PrintNotice();
	}
	else {
		string args[4];
		for (int i = 0; i < argc; i++) {
			args[i] = argv[i];
		}
		if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
			if (args[3] == "2") {
				TranslateToMihoyoBlockUnityFile(args[1], args[2]);
			}
			else {
				TranslateToMihoyoUnityFile(args[1], args[2]);
			}
		}
		else {
			PrintNotice();
		}
	}
}

void PrintNotice() {
	cout << endl;

	cout << "������Unity3d�ļ�תΪMr0k Unity3d�ļ�" << endl;
	cout << "Version: 1.0.0" << endl;
	cout << endl;

	cout << "������ϷΪ: ����:�������" << endl;
	cout << "�����������Ⱥ��������ֹ�⴫������" << endl;
	cout << "ʹ�ý̳��뵽 blog.wtan.xyz Ѱ�� :P" << endl;

	cout << endl;
	cout << "����: <inpath> <outpath> <savefromat>" << endl;
	cout << "inpath : �������ļ�·��" << endl;
	cout << "outpath : ���ܺ��ļ�·��" << endl;
	cout << "savefromat : 1 Ϊ�������һ�����ʽ [��׺��Ϊbundle] " << endl;
	cout << "             2 Ϊ����������⼰�����ʽ [��׺��Ϊblock] " << endl;
	//cout << "             3 Ϊ�������ʽ [��׺��Ϊbundle]" << endl;
}