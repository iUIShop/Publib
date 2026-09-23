#pragma once

#include <windows.h>

namespace IUI
{
	// 判断调用进程是否被提权
	BOOL IsElevated();

	// 判断一个进程是否以管理员权限运行中。
	BOOL IsProcessElevatedAdmin(HANDLE hProcess);
	BOOL IsProcessElevatedAdmin(DWORD dwProcessId);

}
