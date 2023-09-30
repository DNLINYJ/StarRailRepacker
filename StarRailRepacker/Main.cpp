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

	cout << "将正常Unity3d文件转为Mr0k Unity3d文件" << endl;
	cout << "Version: 1.0.0" << endl;
	cout << endl;

	cout << "适用游戏为: 崩坏:星穹铁道" << endl;

	cout << endl;
	cout << "参数: <inpath> <outpath> <savefromat>" << endl;
	cout << "inpath : 待加密文件路径" << endl;
	cout << "outpath : 加密后文件路径" << endl;
	cout << "savefromat : 1 为星穹铁道一二测格式 [后缀名为bundle] " << endl;
	cout << "             2 为星穹铁道三测及公测格式 [后缀名为block] " << endl;
	//cout << "             3 为绝区零格式 [后缀名为bundle]" << endl;
}
