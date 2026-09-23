#include "Debug.h"

#include <sstream>
#include <mutex>
//#include <thread>
//#include <iostream>
//#include <mutex>
//#include <string>
//#include <fstream>
//#include <algorithm>
//#include <windows.h>
//#include <io.h>
//#include <stdio.h>
//#include <direct.h>
#include <shlobj_core.h>
#include <atlstr.h>
#include <minidumpapiset.h>
#include "../Thread/Thread.h"
#include "../String/string.h"

#pragma comment(lib, "Dbghelp.lib")


int IUI::OutputString(const WCHAR* format, ...)
{
	std::locale::global(std::locale(""));

	int nPid = GetCurrentProcessId();
	int nTid = GetCurrentThreadId();

	SYSTEMTIME timeNow;
	GetLocalTime(&timeNow);

	CStringW strArgW;

	va_list	ap;
	va_start(ap, format);
	strArgW.FormatV(format, ap);
	va_end(ap);

	CString strInfo;
	strInfo.Format(L"\r\n<%04d-%02d-%02d %02d:%02d:%02d:%03d>: [%d|%d]: [IUI]: %s\r\n",
		timeNow.wYear, timeNow.wMonth, timeNow.wDay,
		timeNow.wHour, timeNow.wMinute, timeNow.wSecond, timeNow.wMilliseconds,
		nPid, nTid,
		(LPCWSTR)strArgW);

	OutputDebugStringW(strInfo);

	return 0;
}

int IUI::MsgBox(HWND hParent, UINT uType/* = MB_OK*/, const WCHAR* format, ...)
{
	std::locale::global(std::locale(""));

	CStringW strArgW;

	SYSTEMTIME st;
	GetLocalTime(&st);

	va_list	ap;
	va_start(ap, format);
	strArgW.FormatV(format, ap);
	va_end(ap);

	CStringW str;
	str.Format(L"[%d-%02d-%02d %02d:%02d:%02d]: %s",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond,
		(LPCWSTR)strArgW);

	return ::MessageBox(hParent, str, L"Info", uType);
}

std::mutex g_mutexCallStack;
int IUI::GenerateMiniDump(DWORD dwPID, DWORD dwTID, BOOL bSelf, EXCEPTION_POINTERS* pExceptionInfo, LPCTSTR lpszDumpFile)
{
	LONG lr = -1;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	do
	{
		TCHAR szPath[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE);
		PathAppend(szPath, _T("ssx"));
		SHCreateDirectoryEx(NULL, szPath, NULL);
		SYSTEMTIME st;
		GetLocalTime(&st);
		CString strName;
		strName.Format(_T("%s_%04d%02d%02d-%02d%02d%02d-%ld.%ld.dmp"),
			lpszDumpFile,
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond,
			dwPID, dwTID);
		PathAppend(szPath, strName);
		hFile = ::CreateFile(szPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			break;
		}

		MINIDUMP_EXCEPTION_INFORMATION mei;
		mei.ThreadId = dwTID;
		mei.ExceptionPointers = pExceptionInfo;
		mei.ClientPointers = !bSelf;

		HANDLE hDumpProcess = nullptr;
		if (!bSelf)
		{
			hDumpProcess = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
				FALSE, dwPID);
			if (NULL == hDumpProcess)
			{
				break;
			}
		}
		else
		{
			hDumpProcess = GetCurrentProcess();
		}

		// function in DbgHelp.dll is not thread safe.
		BOOL bOk = FALSE;
		{
			std::lock_guard<std::mutex> locker(g_mutexCallStack);

			bOk = MiniDumpWriteDump(
				hDumpProcess,
				dwPID,
				hFile,
				MiniDumpWithFullMemory,
				(NULL == pExceptionInfo) ? NULL : &mei,
				NULL,
				NULL);
		}

		if (!bSelf && NULL != hDumpProcess)
		{
			CloseHandle(hDumpProcess);
			hDumpProcess = NULL;
		}

		if (bOk)
		{
			lr = 0;
		}
	} while (0);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	return lr;
}

IUI::CAutoCSInit g_cs;
LONG IUI::WriteMiniDump(LPCTSTR lpszDumpName, LPCTSTR lpszFileMappingName)
{
	// 接受常驻进程传过来的数据
	DWORD dwPID = 0;
	DWORD dwTID = 0;
	EXCEPTION_POINTERS* pExceptionInfo = NULL;

	// 接受并解析常驻进程传过来的数据是否成功
	BOOL bParseOK = FALSE;

	//
	// 通过内存映射文件，拿到引发异常的常驻进程的信息。
	//
	HANDLE hFileMap = NULL;
	VOID* pView = NULL;
	do
	{
		hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE,
			lpszFileMappingName);
		if (NULL == hFileMap)
		{
			break;
		}

		pView = MapViewOfFile(hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		if (NULL == pView)
		{
			break;
		}

		// 解析pView中的数据
		memcpy(&dwPID, pView, sizeof(DWORD));
		memcpy(&dwTID, (char*)pView + sizeof(DWORD), sizeof(DWORD));
		memcpy(&pExceptionInfo, (char*)pView + sizeof(DWORD) * 2, sizeof(void*));

		bParseOK = TRUE;
	} while (false);

	//
	// 写dump
	//
	LONG lr = -1;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	if (bParseOK)
	{
		do
		{
			TCHAR szPath[MAX_PATH] = { 0 };
			SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE);
			PathAppend(szPath, _T("ssx"));
			SHCreateDirectoryEx(NULL, szPath, NULL);
			SYSTEMTIME st;
			GetLocalTime(&st);//当地时间
			CString strName;
			strName.Format(_T("%s_%04d%02d%02d-%02d%02d%02d-%ld.%ld.dmp"),
				lpszDumpName,
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond,
				dwPID, dwTID);
			PathAppend(szPath, strName);
			hFile = ::CreateFile(szPath,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{
				break;
			}

			MINIDUMP_EXCEPTION_INFORMATION mei;
			mei.ThreadId = dwTID;
			mei.ExceptionPointers = pExceptionInfo;
			// 如果内存位于调用程序的地址空间中，则设置为FALSE（调试器进程）
			// 如果内存驻留在要调试的进程（调试器的目标进程）中，则设置为TRUE。
			mei.ClientPointers = TRUE;

			HANDLE hDumpProcess = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
				FALSE, dwPID);
			if (NULL == hDumpProcess)
			{
				break;
			}

			// DbgHelp.dll中的函数都是非线程安全的。如果多个线程同时崩溃需要写dump，就会有问题，
			// 所以需要做下线程同步。
			BOOL bOk = FALSE;
			{
				CAutoCriticalSection _cs(&g_cs.m_cs);
				bOk = MiniDumpWriteDump(
					hDumpProcess,
					dwPID,
					hFile,
					MiniDumpWithFullMemory,
					(NULL == pExceptionInfo) ? NULL : &mei,
					NULL,
					NULL);
			}

			if (NULL != hDumpProcess)
			{
				CloseHandle(hDumpProcess);
				hDumpProcess = NULL;
			}

			if (bOk)
			{
				lr = 0;
			}
		} while (0);
	}

	//
	// 清理释放
	//
	if (NULL != NULL)
	{
		UnmapViewOfFile(pView);
		pView = NULL;
	}
	if (INVALID_HANDLE_VALUE != hFile)
	{
		::CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if (NULL != hFileMap)
	{
		CloseHandle(hFileMap);
		hFileMap = NULL;
	}

	return lr;
}

BOOL IUI::EnableDebugPrivilege()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}

BOOL IUI::CreateMiniDump(DWORD pid, const wchar_t* dumpFilePath)
{
	// 启用调试权限
	if (!EnableDebugPrivilege())
	{
		return FALSE;
	}

	// 打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess == NULL)
	{
		return FALSE;
	}

	// 创建dump文件
	HANDLE hFile = CreateFile(dumpFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	// 写入minidump
	BOOL result = MiniDumpWriteDump(
		hProcess,
		pid,
		hFile,
		MiniDumpWithFullMemory, // 包含完整内存，根据需要调整类型
		NULL,
		NULL,
		NULL);

	if (!result)
	{
		// 失败
	}

	// 清理
	CloseHandle(hFile);
	CloseHandle(hProcess);

	return result;
}
