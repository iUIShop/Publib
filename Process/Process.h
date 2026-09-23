#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <string>

namespace IUI
{
    DWORD RunAndGetProcessExitCode(LPCTSTR lpszAppPath,
        LPCTSTR lpParameters,
        LPCTSTR lpszDirectory,
        DWORD dwMilliseconds);
    int GetCmdResult(LPCWSTR lpszCmd, std::wstring* pstrResult, const WCHAR* pszCurDir);

	BOOL IsProcessExist(DWORD dwProcessID);
    BOOL IsProcessExist(LPCWSTR lpszProcName, PROCESSENTRY32* pProc);

}
