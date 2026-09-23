#include "ExceptionHandle.h"
#include <Dbghelp.h>
#include <atlstr.h>
#include <strsafe.h>
#include <Shlobj.h>
#include <mutex>
#include <vector>
#include <algorithm>
#include <PathCch.h>
#pragma comment (lib, "Pathcch.lib")
#pragma warning (disable: 4995)


// 生成的Dump目标文件夹
CStringW g_strDumpFilePath;
// 生成的Dump文件名，如果指定"Sharehub"，则生成"Sharehub_c0000005_20241021113610-57052.14228.dmp"，后缀包含异常代码、进程线程日期等信息。
CStringW g_strDumpFileName;
// 目标文件夹允许的Dump文件数量的上限，当已有Dump数量达到上限后，如果有新的Dump文件生成，就自动删除最旧的Dump文件。
int g_nLimitDumpCount = 2;

void* g_pVectorExceptionHandle = nullptr;

// 回调：由用户指定是否生成Dump文件。通常，可以重用日志开关，即日志开关打开的时候，生成dump，否则不生成。
IsEnableExceptionCatchPtr g_fnIsEnableExceptionCatch = nullptr;

#ifndef Min
#define Min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


typedef BOOL(WINAPI* _MiniDumpWriteDump)(
	_In_ HANDLE hProcess,
	_In_ DWORD ProcessId,
	_In_ HANDLE hFile,
	_In_ MINIDUMP_TYPE DumpType,
	_In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	_In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	_In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

typedef bool(*FuncEnumPathCallback)(const WIN32_FIND_DATA* pfd, LPCTSTR lpszRoot, LPCTSTR lpszPath, BOOL bFullPath, BOOL bDir, void* pArg);

bool EnumPathCallback(const WIN32_FIND_DATA* pfd, LPCTSTR lpszRoot, LPCTSTR lpszPath, BOOL bFullPath, BOOL bDir, void* pArg)
{
	if (!bDir)
	{
		std::vector<CStringW> *pvFiles = (std::vector<CStringW> *)pArg;
		if (nullptr != pvFiles)
		{
			pvFiles->push_back(lpszPath);
		}
	}

	return true;
}

// 枚举出lpszRootPath文件夹中所有文件和子文件夹
int EnumPath(
	LPCTSTR lpszRootPath,
	LPCTSTR lpszParentPath,
	__out LONGLONG* pllFolderCount,
	__out LONGLONG* pllFileCount,
	FuncEnumPathCallback fnCallback,
	void* pArg,
	BOOL bFullPath)
{
	if (NULL == lpszParentPath
		|| NULL == lpszRootPath)
	{
		return -1;
	}

	TCHAR szRoot[MAX_PATH] = { 0 };
	TCHAR szFind[MAX_PATH] = { 0 };
	TCHAR szRelativePath[MAX_PATH] = { 0 };
	bool bContinue = true;

	StringCchCopy(szRoot, MAX_PATH, lpszParentPath);
	PathCchAppend(szRoot, MAX_PATH, _T("*"));

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(szRoot, &fd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return -2;
	}

	do
	{
		if (StrCmp(fd.cFileName, _T(".")) == 0
			|| StrCmp(fd.cFileName, _T("..")) == 0)
		{
			continue;
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
			{
				if (NULL != pllFolderCount)
				{
					(*pllFolderCount)++;
				}

				StringCchCopy(szFind, MAX_PATH, lpszParentPath);
				BOOL bRet = PathCchAppend(szFind, MAX_PATH, fd.cFileName);

				if (!bFullPath)
				{
					PathRelativePathTo(szRelativePath,
						lpszRootPath,
						FILE_ATTRIBUTE_DIRECTORY,
						szFind,
						FILE_ATTRIBUTE_NORMAL);
				}

				if (bRet)
				{
					if (nullptr != fnCallback)
					{
						bContinue = fnCallback(&fd, lpszRootPath, bFullPath ? szFind : szRelativePath, bFullPath, TRUE, pArg);
					}

					if (bContinue)
					{
						int nRet = EnumPath(lpszRootPath, szFind, pllFolderCount, pllFileCount, fnCallback, pArg, bFullPath);
						if (nRet > 0)
						{
							bContinue = false;
						}
					}
				}

				if (!bContinue)
				{
					break;
				}
			}
		}
		else
		{
			StringCchCopy(szFind, MAX_PATH, lpszParentPath);
			BOOL bRet = PathCchAppend(szFind, MAX_PATH, fd.cFileName);

			if (!bFullPath)
			{
				TCHAR szOut[MAX_PATH] = { 0 };
				PathRelativePathTo(szOut,
					lpszRootPath,
					FILE_ATTRIBUTE_DIRECTORY,
					szFind,
					FILE_ATTRIBUTE_NORMAL);
				StringCchCopy(szFind, MAX_PATH, szOut);
			}

			if (nullptr != fnCallback)
			{
				bContinue = fnCallback(&fd, lpszRootPath, szFind, bFullPath, FALSE, pArg);
			}

			if (NULL != pllFileCount)
			{
				(*pllFileCount)++;
			}

			if (!bContinue)
			{
				break;
			}
		}
	} while (FindNextFile(hFind, &fd));

	if (INVALID_HANDLE_VALUE != hFind)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	return 0;
}

// 通过参数pnLimit返回允许生成的Dump的最大数量，并把多余的Dump删除
int LimitedDumpCount(LPCTSTR lpszDumpFile, int *pnLimit)
{
	TCHAR szPath[MAX_PATH] = { 0 };
	StringCchCopy(szPath, MAX_PATH, g_strDumpFilePath);
	PathAddBackslash(szPath);

	LONGLONG llFolderCount = 0;
	LONGLONG llFileCount = 0;

	std::vector<CStringW> vFiles;
	EnumPath(szPath, szPath, &llFolderCount, &llFileCount, EnumPathCallback, (void *)(&vFiles), FALSE);

	int nCount = 0;
	std::vector<CStringW> vCurDumps;
	for (auto& file : vFiles)
	{
		file.MakeLower();

		CStringW strDump = lpszDumpFile;
		strDump.MakeLower();

		if (file.Find(strDump) >= 0)
		{
			vCurDumps.push_back(file);
		}
	}

	std::sort(vCurDumps.begin(), vCurDumps.end());

	// 防止无序生长，设置最多10个Dump
	int nLimitCount = g_nLimitDumpCount;
	if (nLimitCount > 10)
	{
		nLimitCount = 10;
	}

	// 如果Dump数量满了，删除最老的Dump
	int nCurDumps = (int)vCurDumps.size();
	int nDeleteCount = Min(nCurDumps - nLimitCount + 1, nCurDumps);
	for (int i = 0; i < nDeleteCount; ++i)
	{
		WCHAR szFile[MAX_PATH] = { 0 };
		StringCchCopyW(szFile, MAX_PATH, szPath);
		PathAppendW(szFile, vCurDumps[i]);
		DeleteFile(szFile);
	}

	if (nullptr != pnLimit)
	{
		*pnLimit = nLimitCount;
	}

	return 0;
}

int GenerateMiniDump(DWORD dwPID, DWORD dwTID, BOOL bSelf, EXCEPTION_POINTERS* pExceptionInfo, LPCTSTR lpszDumpFile)
{
	LONG lr = -1;
	HMODULE hDll = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	do
	{
		WCHAR szDLL[MAX_PATH] = { 0 };
		GetSystemDirectoryW(szDLL, MAX_PATH);
		PathAppendW(szDLL, L"dbghelp.dll");
		hDll = ::LoadLibraryW(szDLL);
		if (NULL == hDll)
		{
			break;
		}

		_MiniDumpWriteDump fnMiniDumpWriteDump = (_MiniDumpWriteDump)::GetProcAddress(hDll,
			"MiniDumpWriteDump");
		if (NULL == fnMiniDumpWriteDump)
		{
			break;
		}

		WCHAR szPath[MAX_PATH] = { 0 };
		StringCchCopy(szPath, MAX_PATH, g_strDumpFilePath);
		SHCreateDirectoryEx(NULL, szPath, NULL);
		SYSTEMTIME st;
		GetLocalTime(&st);
		CStringW strName;
		strName.Format(_T("%s_%04d%02d%02d%02d%02d%02d-%ld.%ld.dmp"),
			PathFindFileName(lpszDumpFile),
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond,
			dwPID, dwTID);
		WCHAR szName[MAX_PATH] = { 0 };
		StringCchCopy(szName, MAX_PATH, strName);
		PathCchAppend(szPath, MAX_PATH, szName);
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
			bOk = fnMiniDumpWriteDump(
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
	if (NULL != hDll)
	{
		FreeLibrary(hDll);
		hDll = NULL;
	}

	return lr;
}

LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
	if (nullptr == pExceptionInfo
		|| nullptr == pExceptionInfo->ContextRecord
		|| nullptr == pExceptionInfo->ExceptionRecord)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// filter DBG_PRINTEXCEPTION_C, DBG_PRINTEXCEPTION_WIDE_C exception, its throw by OutputDebugString
	if (DBG_PRINTEXCEPTION_C == pExceptionInfo->ExceptionRecord->ExceptionCode
		|| /*DBG_PRINTEXCEPTION_WIDE_C*/0x4001000A == pExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (EXCEPTION_ACCESS_VIOLATION == pExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		// EXCEPTION_ACCESS_VIOLATION
		// such as: char *p = nullptr; *p = 1;
		// can raise exception repeated.
		// so, Remove Vectored Exception Handler.
		if (nullptr != g_pVectorExceptionHandle)
		{
			RemoveVectoredExceptionHandler(g_pVectorExceptionHandle);
			g_pVectorExceptionHandle = nullptr;
		}
	}

	if (// DBG_CONTROL_C/*0x40010005L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
		//|| DBG_CONTROL_BREAK/*0x40010008L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
		EXCEPTION_ACCESS_VIOLATION/*0xC0000005L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
		|| EXCEPTION_INVALID_HANDLE/*0xC0000008L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
		|| STATUS_ASSERTION_FAILURE/*0xC0000420L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
		|| 0xE073616E /*Sanitizer error detected */ == pExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		// 保护生成Dump文件过程，因为可能同时有多个线程崩溃，多个线程同时生成Dump文件
		static std::mutex g_mutexGenerateDumpFile;
		std::lock_guard<std::mutex> locker(g_mutexGenerateDumpFile);

		std::string status;
		if (nullptr != g_fnIsEnableExceptionCatch && g_fnIsEnableExceptionCatch())
		{
			DWORD dwPID = GetCurrentProcessId();
			DWORD dwTID = GetCurrentThreadId();

			CStringW strName;
			strName.Format(L"%s_%x", (LPCWSTR)g_strDumpFileName, pExceptionInfo->ExceptionRecord->ExceptionCode);

			int nLimit = 2;
			LimitedDumpCount(strName, &nLimit);

			if (nLimit > 0)
			{
				GenerateMiniDump(dwPID, dwTID, TRUE, pExceptionInfo, strName);
			}
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

int VectorHandle(LPCWSTR lpszDumpPath, LPCWSTR lpszDumpName, int nLimitDumpCount, IsEnableExceptionCatchPtr fn)
{
	g_strDumpFilePath = lpszDumpPath;
	g_strDumpFileName = lpszDumpName;
	g_nLimitDumpCount = nLimitDumpCount;
	g_fnIsEnableExceptionCatch = fn;
	g_pVectorExceptionHandle = AddVectoredExceptionHandler(TRUE, VectoredExceptionHandler);

	return 0;
}

int UnVectorHandle()
{
	if (nullptr != g_pVectorExceptionHandle)
	{
		RemoveVectoredExceptionHandler(g_pVectorExceptionHandle);
		g_pVectorExceptionHandle = nullptr;
	}

	return 0;
}
