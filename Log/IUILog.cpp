#include "IUILog.h"
#include <iostream>
#include <strsafe.h>
#include "../String/String.h"

#define CAN_WRITE_LOG_MUTEX		_T("Global\\CAN_WRITE_LOG_MUTEX_")

void AutoReleaseMutex(HANDLE *phMutex)
{
	if (nullptr != phMutex)
	{
		::ReleaseMutex(*phMutex);
	}
}


CIUIProcessLog::CIUIProcessLog()
	: m_hLogFile(INVALID_HANDLE_VALUE)
	, m_bEnable(TRUE)
	, m_dwMaxFilesize(10000000)// 10MB
	, m_dwLogOutputMode(LOG_OUTPUT_MODE_FILE)
{
}

CIUIProcessLog::~CIUIProcessLog()
{
	Close();
}

int CIUIProcessLog::Open(LPCWSTR lpszFile)
{
	if (NULL == lpszFile)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (INVALID_HANDLE_VALUE != m_hLogFile)
	{
		DWORD dwErr = GetLastError();
		CStringW strErr;
		strErr.Format(L"日志文件'%s'已创建，不需要重复创建.\r\n", lpszFile);

		return ERROR_ALREADY_EXISTS;
	}

	m_hLogFile = CreateFileW(lpszFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == m_hLogFile)
	{
		DWORD dwErr = GetLastError();
		CStringW strErr;
		strErr.Format(L"打开或创建日志文件'%s'失败，失败码：0n%d.\r\n", lpszFile, dwErr);
		OutputDebugStringW(strErr);

		return dwErr;
	}

	// 不能使用动态创建的guid来作为互斥名的一部分。
	// 因为用于多进程安全日志的时候，不同进程，创建出来的guid不一样。
	// 所以使用文件路径作为互斥名
	WCHAR szShortPathW[MAX_PATH] = { 0 };
	GetShortPathNameW(lpszFile, szShortPathW, MAX_PATH);
	CharLowerBuffW(szShortPathW, MAX_PATH);
	// 把路径中的冒号和斜杠替换为下下划线
	for (int i = 0; i < MAX_PATH; ++i)
	{
		if (szShortPathW[i] == ':')
		{
			szShortPathW[i] = '-';
		}
		if (szShortPathW[i] == '\\')
		{
			szShortPathW[i] = '_';
		}
	}

	WCHAR szMutexNameW[MAX_PATH] = { 0 };
	StringCchCopyW(szMutexNameW, MAX_PATH, L"Global\\");
	StringCchCatW(szMutexNameW, MAX_PATH, szShortPathW);

	// 创建互斥
	// 当第二个参数为TRUE时，调用CreateMutexW的线程拥有互斥，其它线程在本线程ReleaseMutex之前是等待不到互斥的。
	// 当第二个参数为FALSE时，调用CreateMutexW的线程不拥有互斥，其它线程可以立即等到互斥。
	// 当互斥已经被提前创建出来，调用CreateMutexW后会立即返回，所以需要判断返回值，如果是互斥已创建，需要等待互斥有信号后再写入文件头。
	m_hCanWriteMutex = CreateMutexW(NULL, TRUE, szMutexNameW);
	DWORD dwRet = GetLastError();
	std::unique_ptr<HANDLE, void(*)(HANDLE *)> upMutex(&m_hCanWriteMutex, AutoReleaseMutex);
	if (ERROR_ALREADY_EXISTS == dwRet)
	{
		WaitForSingleObject(m_hCanWriteMutex, INFINITE);
	}

	m_strLogFileW = lpszFile;

	{
		// unicode 版本，写入文件头
		LARGE_INTEGER file_size;

		file_size.QuadPart = 0;
		file_size.LowPart = ::GetFileSize(m_hLogFile, (LPDWORD)&file_size.HighPart);

		if (file_size.QuadPart == 0)
		{
			DWORD tag = 0xFEFF;
			DWORD nHave = 0;
			BOOL bRet = WriteFile(m_hLogFile, &tag, 2, &nHave, NULL);
			if (!bRet)
			{
				DWORD dwErr = GetLastError();
				CStringW strErr;
				strErr.Format(L"写日志文件'%s'的Unicode头失败，失败码：0n%d.\r\n", (LPCWSTR)m_strLogFileW, dwErr);

				OutputDebugStringW(strErr);
				return dwErr;
			}
		}
	}

	SetFilePointer(m_hLogFile, 0, 0, FILE_END);

	return ERROR_SUCCESS;
}

int CIUIProcessLog::Close()
{
	if (NULL != m_hCanWriteMutex)
	{
		WaitForSingleObject(m_hCanWriteMutex, 3000);
	}
	if (INVALID_HANDLE_VALUE != m_hLogFile)
	{
		CloseHandle(m_hLogFile);
		m_hLogFile = INVALID_HANDLE_VALUE;
	}
	if (NULL != m_hCanWriteMutex)
	{
		ReleaseMutex(m_hCanWriteMutex);
		CloseHandle(m_hCanWriteMutex);
		m_hCanWriteMutex = NULL;
	}

	return 0;
}

int CIUIProcessLog::SetMaxLogFilesize(DWORD dwFilesize)
{
	m_dwMaxFilesize = dwFilesize;
	return 0;
}

int CIUIProcessLog::SetOutputMode(DWORD dwLogOutputMode)
{
	m_dwLogOutputMode = dwLogOutputMode;
	return 0;
}

BOOL CIUIProcessLog::Enable(BOOL bEnable)
{
	BOOL bOld = m_bEnable;
	m_bEnable = bEnable;

	return bOld;
}

// CString不支持%S
// 之所以WriteA不直接调用WriteW，是因为预防字符串里的%被当成转义符。
int CIUIProcessLog::WriteA(LPCSTR fmt, ...)
{
	if (NULL == fmt)
	{
		OutputDebugStringW(L"CIUIProcessLog::Write fmt == NULL.\r\n");
		return -1;
	}
	if (m_bEnable == FALSE)
	{
		OutputDebugStringW(L"CIUIProcessLog::Write m_bEnable == FALSE.\r\n");
		return -2;
	}

	if (LOG_OUTPUT_MODE_FILE & m_dwLogOutputMode)
	{
		if (INVALID_HANDLE_VALUE == m_hLogFile)
		{
			OutputDebugStringW(L"CIUIProcessLog::Write INVALID_HANDLE_VALUE == m_hLogFile.\r\n");
			return -3;
		}
	}

	CStringA strArgA;

	va_list	ap;
	va_start(ap, fmt);
	strArgA.FormatV(fmt, ap);
	va_end(ap);

	CStringW strArgW = IUI::AnsiToUnicode(strArgA).c_str();
	return WriteStringW(strArgW);

	return 0;
}

int CIUIProcessLog::WriteW(LPCWSTR fmt, ...)
{
	if (NULL == fmt)
	{
		OutputDebugStringW(L"CIUIProcessLog::Write fmt == NULL.\r\n");
		return -1;
	}
	if (m_bEnable == FALSE)
	{
		OutputDebugStringW(L"CIUIProcessLog::Write m_bEnable == FALSE.\r\n");
		return -2;
	}

	if (LOG_OUTPUT_MODE_FILE & m_dwLogOutputMode)
	{
		if (INVALID_HANDLE_VALUE == m_hLogFile)
		{
			OutputDebugStringW(L"CIUIProcessLog::Write INVALID_HANDLE_VALUE == m_hLogFile.\r\n");
			return -3;
		}
	}

	CStringW strArgW;

	va_list	ap;
	va_start(ap, fmt);
	strArgW.FormatV(fmt, ap);
	va_end(ap);

	WriteStringW(strArgW);

	return 0;
}

int CIUIProcessLog::WriteStringW(LPCWSTR lpszLog)
{
	SYSTEMTIME timeNow;
	GetLocalTime(&timeNow);

	CStringW strLogW;
	strLogW.Format(L"%04d-%02d-%02d %02d:%02d:%02d:%03d\t[%d][%d]\t%s\r\n",
		timeNow.wYear, timeNow.wMonth, timeNow.wDay,
		timeNow.wHour, timeNow.wMinute, timeNow.wSecond, timeNow.wMilliseconds,
		GetCurrentProcessId(), GetCurrentThreadId(), lpszLog);

	BOOL bRet = TRUE;

	if (m_dwLogOutputMode & LOG_OUTPUT_MODE_FILE)
	{
		// 进程间同步。这里不能使用Event，因为拥有Event的线程意外退出后，
		// 操作系统不会自动把事件置为有状态，这样其它等待线程就等不到了。
		// 但互斥会。
		// 没有等到写互斥的话，会产生脏数据，但比丢弃强。
		DWORD dwWait = WaitForSingleObject(m_hCanWriteMutex, 60000);
		std::unique_ptr<HANDLE, void(*)(HANDLE *)> upMutex(&m_hCanWriteMutex, AutoReleaseMutex);

		do
		{
			if (INVALID_HANDLE_VALUE == m_hLogFile)
			{
				break;
			}

			// 因为可能多进程写同一个文件，所以每次写之前，把文件指针定位到最后。
			SetFilePointer(m_hLogFile, 0, 0, FILE_END);

			if (m_dwMaxFilesize)
			{
				LARGE_INTEGER lFileSize;
				lFileSize.QuadPart = 0;
				GetFileSizeEx(m_hLogFile, &lFileSize);

				if (lFileSize.QuadPart > m_dwMaxFilesize)
				{
					lFileSize.QuadPart = 2;
					::SetFilePointer(m_hLogFile,
						lFileSize.LowPart, &lFileSize.HighPart, FILE_BEGIN);
					::SetEndOfFile(m_hLogFile);
				}
			}

			DWORD nHave = 0;
			bRet = WriteFile(m_hLogFile,
				(LPCWSTR)strLogW, strLogW.GetLength() * sizeof(WCHAR), &nHave, NULL);
			if (!bRet)
			{
				DWORD dwErr = GetLastError();
				CStringW strErrW;
				strErrW.Format(L"写日志文件'%s'，内容'%s'失败，失败码：0n%d.\r\n",
					(LPCWSTR)m_strLogFileW, (LPCWSTR)strLogW, dwErr);
				OutputDebugStringW(strErrW);

				break;
			}

			FlushFileBuffers(m_hLogFile);

		} while (FALSE);

		// 智能指针释放互斥
		// 没有正常等到写互斥锁也释放互斥。否则拥有互斥的进程没有释放互斥时就
		// 崩溃了的话，其它进程的线程以后只能永远等超时才能写日志了（不过经过测试，发现
		// 如果用事件的话，进程崩溃后不会把事件自动设置为有信息，但互斥可以）。
	}
	else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_STDIO)
	{
		std::wcout << (LPCWSTR)strLogW;
	}
	else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_DEBUGER)
	{
		OutputDebugStringW(strLogW);
	}

	return 0;
}

int CIUIProcessLog::InsertSpaceLine()
{
	BOOL bRet = TRUE;

	CStringW strLogW = L"\r\n";

	if (m_dwLogOutputMode & LOG_OUTPUT_MODE_FILE)
	{
		// 进程间同步。这里不能使用Event，因为拥有Event的线程意外退出后，
		// 操作系统不会自动把事件置为有状态，这样其它等待线程就等不到了。
		// 但互斥会。
		// 没有等到写互斥的话，会产生脏数据，但比丢弃强。
		DWORD dwWait = WaitForSingleObject(m_hCanWriteMutex, 60000);
		std::unique_ptr<HANDLE, void(*)(HANDLE *)> upMutex(&m_hCanWriteMutex, AutoReleaseMutex);

		do
		{
			// 因为可能多进程写同一个文件，所以每次写之前，把文件指针定位到最后。
			SetFilePointer(m_hLogFile, 0, 0, FILE_END);

			if (m_dwMaxFilesize)
			{
				LARGE_INTEGER lFileSize;
				lFileSize.QuadPart = 0;
				GetFileSizeEx(m_hLogFile, &lFileSize);

				if (lFileSize.QuadPart > m_dwMaxFilesize)
				{
					lFileSize.QuadPart = 2;
					::SetFilePointer(m_hLogFile,
						lFileSize.LowPart, &lFileSize.HighPart, FILE_BEGIN);
					::SetEndOfFile(m_hLogFile);
				}
			}

			DWORD nHave = 0;
			bRet = WriteFile(m_hLogFile, (LPCWSTR)strLogW, strLogW.GetLength() * sizeof(WCHAR), &nHave, NULL);
			if (!bRet)
			{
				DWORD dwErr = GetLastError();
				CStringW strErrW;
				strErrW.Format(L"写日志文件'%s'，内容'%s'失败，失败码：0n%d.\r\n",
					(LPCWSTR)m_strLogFileW, (LPCWSTR)strLogW, dwErr);
				OutputDebugStringW(strErrW);

				break;
			}

			FlushFileBuffers(m_hLogFile);

		} while (FALSE);

		// 智能指针释放互斥
		// 没有正常等到写互斥锁也释放互斥。否则拥有互斥的进程没有释放互斥时就
		// 崩溃了的话，其它进程的线程以后只能永远等超时才能写日志了（不过经过测试，发现
		// 如果用事件的话，进程崩溃后不会把事件自动设置为有信息，但互斥可以）。
	}
	else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_STDIO)
	{
	}
	else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_DEBUGER)
	{
		OutputDebugStringW(strLogW);
	}

	return 0;
}

CIUILog::CIUILog()
	: m_hLogFile(INVALID_HANDLE_VALUE)
	, m_bEnable(TRUE)
	, m_dwMaxFilesize(10000000)// 10MB
	, m_dwLogOutputMode(LOG_OUTPUT_MODE_FILE)
{
}

CIUILog::~CIUILog()
{
	Close();
}

int CIUILog::Open(LPCWSTR lpszFile)
{
	if (NULL == lpszFile)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (INVALID_HANDLE_VALUE != m_hLogFile)
	{
		DWORD dwErr = GetLastError();
		CStringW strErr;
		strErr.Format(L"日志文件'%s'已创建，不需要重复创建.\r\n", lpszFile);

		return ERROR_ALREADY_EXISTS;
	}

	m_hLogFile = CreateFileW(lpszFile, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == m_hLogFile)
	{
		DWORD dwErr = GetLastError();
		CStringW strErr;
		strErr.Format(L"打开或创建日志文件'%s'失败，失败码：0n%d.\r\n", lpszFile, dwErr);
		OutputDebugStringW(strErr);

		return dwErr;
	}

	m_strLogFileW = lpszFile;

	{
		// unicode 版本，写入文件头
		LARGE_INTEGER file_size;

		file_size.QuadPart = 0;
		file_size.LowPart = ::GetFileSize(m_hLogFile, (LPDWORD)&file_size.HighPart);

		if (file_size.QuadPart == 0)
		{
			DWORD tag = 0xFEFF;
			DWORD nHave = 0;
			BOOL bRet = WriteFile(m_hLogFile, &tag, 2, &nHave, NULL);
			if (!bRet)
			{
				DWORD dwErr = GetLastError();
				CStringW strErr;
				strErr.Format(L"写日志文件'%s'的Unicode头失败，失败码：0n%d.\r\n", (LPCWSTR)m_strLogFileW, dwErr);

				OutputDebugStringW(strErr);
				return dwErr;
			}
		}
	}

	SetFilePointer(m_hLogFile, 0, 0, FILE_END);

	m_threadWirte = std::thread(&CIUILog::WriteThread, this);

	return ERROR_SUCCESS;
}

int CIUILog::Close()
{
	if (INVALID_HANDLE_VALUE != m_hLogFile)
	{
		CloseHandle(m_hLogFile);
		m_hLogFile = INVALID_HANDLE_VALUE;
	}

	return 0;
}

int CIUILog::SetMaxLogFilesize(DWORD dwFilesize)
{
	m_dwMaxFilesize = dwFilesize;
	return 0;
}

int CIUILog::SetOutputMode(DWORD dwLogOutputMode)
{
	m_dwLogOutputMode = dwLogOutputMode;
	return 0;
}

BOOL CIUILog::Enable(BOOL bEnable)
{
	BOOL bOld = m_bEnable;
	m_bEnable = bEnable;

	return bOld;
}

// CString不支持%S
// 之所以WriteA不直接调用WriteW，是因为预防字符串里的%被当成转义符。
int CIUILog::WriteA(LPCSTR fmt, ...)
{
	if (NULL == fmt)
	{
		OutputDebugStringW(L"CIUILog::Write fmt == NULL.\r\n");
		return -1;
	}
	if (m_bEnable == FALSE)
	{
		OutputDebugStringW(L"CIUILog::Write m_bEnable == FALSE.\r\n");
		return -2;
	}

	if (LOG_OUTPUT_MODE_FILE & m_dwLogOutputMode)
	{
		if (INVALID_HANDLE_VALUE == m_hLogFile)
		{
			OutputDebugStringW(L"CIUILog::Write INVALID_HANDLE_VALUE == m_hLogFile.\r\n");
			return -3;
		}
	}

	CStringA strArgA;

	va_list	ap;
	va_start(ap, fmt);
	strArgA.FormatV(fmt, ap);
	va_end(ap);

	CStringW strArgW = IUI::AnsiToUnicode(strArgA).c_str();
	return WriteStringW(strArgW);

	return 0;
}

int CIUILog::WriteW(LPCWSTR fmt, ...)
{
	if (NULL == fmt)
	{
		OutputDebugStringW(L"CIUILog::Write fmt == NULL.\r\n");
		return -1;
	}
	if (m_bEnable == FALSE)
	{
		OutputDebugStringW(L"CIUILog::Write m_bEnable == FALSE.\r\n");
		return -2;
	}

	if (LOG_OUTPUT_MODE_FILE & m_dwLogOutputMode)
	{
		if (INVALID_HANDLE_VALUE == m_hLogFile)
		{
			OutputDebugStringW(L"CIUILog::Write INVALID_HANDLE_VALUE == m_hLogFile.\r\n");
			return -3;
		}
	}

	CStringW strArgW;

	va_list	ap;
	va_start(ap, fmt);
	strArgW.FormatV(fmt, ap);
	va_end(ap);

	WriteStringW(strArgW);

	return 0;
}

int CIUILog::WriteStringW(LPCWSTR lpszLog)
{
	SYSTEMTIME timeNow;
	GetLocalTime(&timeNow);

	CStringW strLogW;
	strLogW.Format(L"%04d-%02d-%02d %02d:%02d:%02d:%03d\t[%d][%d]\t%s\r\n",
		timeNow.wYear, timeNow.wMonth, timeNow.wDay,
		timeNow.wHour, timeNow.wMinute, timeNow.wSecond, timeNow.wMilliseconds,
		GetCurrentProcessId(), GetCurrentThreadId(), lpszLog);

	m_qLogs.Put(strLogW);

	return 0;
}

int CIUILog::InsertSpaceLine()
{
	BOOL bRet = TRUE;

	CStringW strLogW = L"\r\n";

	m_qLogs.Put(strLogW);

	return 0;
}

int CIUILog::WriteThread()
{
	BOOL bRet = TRUE;

	while (!m_bThreadExit)
	{
		CStringW strLogW;
		m_qLogs.Take(strLogW);

		if (m_dwLogOutputMode & LOG_OUTPUT_MODE_FILE)
		{
			do
			{
				if (INVALID_HANDLE_VALUE == m_hLogFile)
				{
					break;
				}

				// 所以每次写之前，把文件指针定位到最后。
				SetFilePointer(m_hLogFile, 0, 0, FILE_END);

				if (m_dwMaxFilesize)
				{
					LARGE_INTEGER lFileSize;
					lFileSize.QuadPart = 0;
					GetFileSizeEx(m_hLogFile, &lFileSize);

					if (lFileSize.QuadPart > m_dwMaxFilesize)
					{
						lFileSize.QuadPart = 2;
						::SetFilePointer(m_hLogFile,
							lFileSize.LowPart, &lFileSize.HighPart, FILE_BEGIN);
						::SetEndOfFile(m_hLogFile);
					}
				}

				DWORD nHave = 0;
				bRet = WriteFile(m_hLogFile,
					(LPCWSTR)strLogW, strLogW.GetLength() * sizeof(WCHAR), &nHave, NULL);
				if (!bRet)
				{
					DWORD dwErr = GetLastError();
					CStringW strErrW;
					strErrW.Format(L"写日志文件'%s'，内容'%s'失败，失败码：0n%d.\r\n",
						(LPCWSTR)m_strLogFileW, (LPCWSTR)strLogW, dwErr);
					OutputDebugStringW(strErrW);

					break;
				}

				FlushFileBuffers(m_hLogFile);

			} while (FALSE);
		}
		else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_STDIO)
		{
			std::wcout << (LPCWSTR)strLogW;
		}
		else if (m_dwLogOutputMode & LOG_OUTPUT_MODE_DEBUGER)
		{
			OutputDebugStringW(strLogW);
		}
	}

	return 0;
}


int GetRunningFolder(TCHAR szFolder[MAX_PATH])
{
	GetModuleFileName(nullptr, szFolder, MAX_PATH);
	PathRemoveFileSpec(szFolder);

	return 0;
}

void write_log(const WCHAR* format, ...)
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
    strInfo.Format(L"\r\n<%04d-%02d-%02d %02d:%02d:%02d:%03d>: [%d|%d]: %s",
        timeNow.wYear, timeNow.wMonth, timeNow.wDay,
        timeNow.wHour, timeNow.wMinute, timeNow.wSecond, timeNow.wMilliseconds,
        nPid, nTid,
        (LPCWSTR)strArgW);

    OutputDebugStringW(strInfo);
}
