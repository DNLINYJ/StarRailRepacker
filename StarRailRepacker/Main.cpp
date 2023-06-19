#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "Translate.h"

void main(int argc, char** argv) {
	if (argc == 1) {
		cout << "将正常Unity3d文件转为Mr0k Unity3d文件 By:菠萝小西瓜" << endl;
		cout << "适用游戏为: 崩坏:星穹铁道一二测 绝区零一测" << endl;
		cout << "本程序仅在内群传播，禁止外传！！！" << endl;
		cout << endl;
		cout << "参数: <inpath> <outpath> <savefromat>" << endl;
		cout << "inpath : 待加密文件路径" << endl;
		cout << "outpath : 加密后文件路径" << endl;
		cout << "savefromat : 1 为星穹铁道格式 2 为绝区零格式 [默认生成星穹铁道形式]" << endl;
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
			cout << "将正常Unity3d文件转为Mr0k Unity3d文件 By:菠萝小西瓜" << endl;
			cout << "适用游戏为: 崩坏:星穹铁道一二测 绝区零一测" << endl;
			cout << "本程序仅在内群传播，禁止外传！！！" << endl;
			cout << endl;
			cout << "参数: <inpath> <outpath> <savefromat>" << endl;
			cout << "inpath : 待加密文件路径" << endl;
			cout << "outpath : 加密后文件路径" << endl;
			cout << "savefromat : 1 为星穹铁道格式 2 为绝区零格式 [默认生成星穹铁道形式]" << endl;
		}
	}
}