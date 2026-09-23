#include "Process.h"
#include <crtdbg.h>
#include <shlwapi.h>
#include <memory>
#include <strsafe.h>
#include <comutil.h>

#pragma comment (lib, "comsuppw.lib")

void ReleaseHandle(HANDLE* ph)
{
	if (nullptr != ph && nullptr != *ph)
	{
		CloseHandle(*ph);
	}
}

DWORD IUI::RunAndGetProcessExitCode(LPCTSTR lpszAppPath,
    LPCTSTR lpParameters,
    LPCTSTR lpszDirectory,
    DWORD dwMilliseconds)
{
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.lpFile = lpszAppPath;
    ShExecInfo.lpParameters = lpParameters;
    ShExecInfo.lpDirectory = lpszDirectory;
    ShExecInfo.nShow = SW_HIDE;
    ShellExecuteEx(&ShExecInfo);

    if (WaitForSingleObject(ShExecInfo.hProcess, dwMilliseconds) == WAIT_TIMEOUT)
    {
        TerminateProcess(ShExecInfo.hProcess, 0);
        return -1;
    }

    DWORD dwExitCode;
    BOOL bOK = GetExitCodeProcess(ShExecInfo.hProcess, &dwExitCode);
    _ASSERT(bOK);

    return dwExitCode;
}

int IUI::GetCmdResult(LPCWSTR lpszCmd, std::wstring* pstrResult, const WCHAR* pszCurDir)
{
#define CMD_MAX_LEN		0x7FFF

	int nRet = 0;
	HANDLE hRead = INVALID_HANDLE_VALUE;
	std::unique_ptr<HANDLE, void(*)(HANDLE*)> upReadHandle(&hRead, ReleaseHandle);

	HANDLE hWrite = INVALID_HANDLE_VALUE;
	std::unique_ptr<HANDLE, void(*)(HANDLE*)> upWriteHandle(&hWrite, ReleaseHandle);

	// 当创建进程的时候，用于保存命令行；
	// 当进程执行完后，用于读取管理的缓存区。
	WCHAR* pszCmd = NULL;

	do
	{
		if (NULL == lpszCmd || NULL == pstrResult)
		{
			nRet = -1;
			break;
		}

		pstrResult->clear();

		//
		// 创建匿名管道。执行结果将写入管道。
		//

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		if (!CreatePipe(&hRead, &hWrite, &sa, 10000000)) // 指定10MB缓冲，如果设置为0，使用默认缓存，当命令结果较长时，可能WaitForSingleObject会超时。
		{
			_ASSERT(FALSE);
			nRet = -2;
			break;
		}

		//
		// 执行命令
		//
		pszCmd = new WCHAR[CMD_MAX_LEN];
		std::unique_ptr<WCHAR, void(*)(WCHAR*)> upszCmd(pszCmd, [](WCHAR* p) {if (nullptr != p) delete[] p; });
		memset(pszCmd, 0, sizeof(WCHAR) * CMD_MAX_LEN);
		StringCchCopyW(pszCmd, CMD_MAX_LEN, lpszCmd);

		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(STARTUPINFOW));
		si.cb = sizeof(STARTUPINFOW);
		GetStartupInfoW(&si);
		si.hStdError = hWrite;
		si.hStdOutput = hWrite; // 新创建进程的标准输出连在写管道一端
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

		PROCESS_INFORMATION pi;
		if (!CreateProcessW(NULL, pszCmd, NULL, NULL, TRUE, NULL, NULL, pszCurDir, &si, &pi))
		{
			_ASSERT(FALSE);
			nRet = -3;
			break;
		}

		//
		// 等待命令执行完毕
		//
		DWORD dwRet = WaitForSingleObject(pi.hProcess, 10 * 1000);
		switch (dwRet)
		{
		case WAIT_TIMEOUT:
		case WAIT_FAILED:
			nRet = -4;
			break;

		case WAIT_OBJECT_0:
			if (INVALID_HANDLE_VALUE != hWrite)
			{
				upWriteHandle.reset();
				hWrite = INVALID_HANDLE_VALUE;
			}
			break;

		default:
			_ASSERT(FALSE);
			nRet = -4;
			break;
		}
		if (NULL != pi.hProcess)
		{
			CloseHandle(pi.hProcess);
			pi.hProcess = NULL;
		}
		if (NULL != pi.hThread)
		{
			CloseHandle(pi.hThread);
			pi.hThread = NULL;
		}

		if (0 > nRet)
		{
			break;
		}

		DWORD dwRead = 0;
		BOOL bRet = FALSE;
		//
		// 读结果
		//
		std::string strResult;
		while (true)
		{
			dwRead = 0;
			bRet = FALSE;
			memset(pszCmd, 0, sizeof(WCHAR) * CMD_MAX_LEN);

			//读取管道
			bRet = ReadFile(hRead, (CHAR*)pszCmd, CMD_MAX_LEN - 1, &dwRead, NULL);
			if (!bRet || 0 == dwRead)
			{
				break;
			}

			strResult += (CHAR*)pszCmd;

			Sleep(1);
		}

		*pstrResult = (LPCWSTR)_bstr_t(strResult.c_str());

		// pstrResult就是输出。
	} while (false);

	return nRet;
}

BOOL IUI::IsProcessExist(DWORD dwProcessID)
{
	HANDLE hProcess = nullptr;
	BOOL bExist = TRUE;

	do
	{
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessID);
		if (nullptr == hProcess)
		{
			bExist = FALSE;
			break;
		}

		// 有时候，即使在任务管理器中，已经找不到该进程了，但是OpenProcess仍然返回了一个有效的句柄。
		DWORD dwExitCode = STILL_ACTIVE;
		BOOL bSuccess = GetExitCodeProcess(hProcess, &dwExitCode);
		if (bSuccess && STILL_ACTIVE != dwExitCode)
		{
			bExist = FALSE;
			break;
		}

	} while (FALSE);

	if (nullptr == hProcess)
	{
		CloseHandle(hProcess);
		hProcess = nullptr;
	}

	return bExist;
}

// 基于AI
BOOL IUI::IsProcessExist(LPCWSTR lpszProcName, PROCESSENTRY32 *pProc)
{
	BOOL bExist = FALSE;
	{
		// Take a snapshot of all processes in the system.
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hProcessSnap)
		{
			return bExist;
		}

		// Set the size of the structure before using it.
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);

		// Retrieve information about the first process,
		// and exit if unsuccessful
		if (!Process32First(hProcessSnap, &pe32))
		{
			CloseHandle(hProcessSnap);
			hProcessSnap = INVALID_HANDLE_VALUE;
			return bExist;
		}

		// Now walk the snapshot of processes, and
		// display information about each process in turn
		do
		{
			// Retrieve the priority class.
			DWORD dwPriorityClass = 0;
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			if (NULL == hProcess)
			{
			}
			else
			{
				dwPriorityClass = GetPriorityClass(hProcess);
				if (0 == dwPriorityClass)
				{
				}
				CloseHandle(hProcess);
			}

			if (StrCmpI(pe32.szExeFile, lpszProcName) == 0)
			{
				bExist = TRUE;

				if (nullptr != pProc)
				{
					memcpy(pProc, &pe32, sizeof(PROCESSENTRY32));
				}

				break;
			}

		} while (Process32Next(hProcessSnap, &pe32));

		if (INVALID_HANDLE_VALUE != hProcessSnap)
		{
			CloseHandle(hProcessSnap);
			hProcessSnap = INVALID_HANDLE_VALUE;
		}

		return bExist;
	}

}
