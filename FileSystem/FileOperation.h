#pragma once

#include <windows.h>

namespace IUI
{
	// 拷贝文件夹，如果目标文件夹已存在，则合并
	int CopyFolderW(LPCWSTR lpszSource, LPCWSTR lpszDest);
}
