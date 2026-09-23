#pragma once
#include <windows.h>

namespace IUI
{
	int OutputString(const WCHAR* format, ...);
	int MsgBox(HWND hParent, UINT uType/* = MB_OK*/, const WCHAR* format, ...);

	// 崩溃的程序自己调用生成Dump
	int GenerateMiniDump(DWORD dwPID, DWORD dwTID, BOOL bSelf, EXCEPTION_POINTERS* pExceptionInfo, LPCTSTR lpszDumpFile);

	LONG WriteMiniDump(LPCTSTR lpszDumpName, LPCTSTR lpszFileMappingName);

	BOOL EnableDebugPrivilege();
	BOOL CreateMiniDump(DWORD pid, const wchar_t* dumpFilePath);
}
