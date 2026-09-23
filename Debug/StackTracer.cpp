#include "StackTracer.h"
#include <sstream>
#include <tchar.h>
#include <shlwapi.h>
#include <strsafe.h>

#pragma warning(push)
#pragma warning(disable : 4091)
#pragma warning(pop)

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "shlwapi.lib")

#ifndef STATUS_POSSIBLE_DEADLOCK
#define STATUS_POSSIBLE_DEADLOCK         ((NTSTATUS)0xC0000194L)
#endif


// Translate exception code to description
#define CODE_DESCR(code) (#code)


StackFrame::StackFrame()
{
	pszFunctionName = new char[MAX_SYM_NAME];
	SecureZeroMemory(pszFunctionName, sizeof(char) * MAX_SYM_NAME);
}

StackFrame::~StackFrame()
{
	if (nullptr != pszFunctionName)
	{
		delete[] pszFunctionName;
		pszFunctionName = nullptr;
	}
}

void StackFrame::Clear()
{
	hFuncAddress = nullptr;
	hModuleBaseAddr = nullptr;
	hModuleEndAddr = nullptr;
	SecureZeroMemory(szModuleName, sizeof(char) * MAX_PATH);
	SecureZeroMemory(pszFunctionName, sizeof(char) * MAX_SYM_NAME);
	SecureZeroMemory(szFileName, sizeof(char) * MAX_PATH);
	nLineNumber = 0;
}


StackTracer::StackTracer(void)
	:m_dwExceptionCode(0)
{
	// Get machine type
	m_dwMachineType = 0;
	size_t Count = 256;
	TCHAR wszProcessor[256] = { 0 };
	::_tgetenv_s(&Count, wszProcessor, _T("PROCESSOR_ARCHITECTURE"));

	if (nullptr != wszProcessor)
	{
		if ((!wcscmp(_T("EM64T"), wszProcessor)) || !wcscmp(_T("AMD64"), wszProcessor))
		{
			m_dwMachineType = IMAGE_FILE_MACHINE_AMD64;
		}
		else if (!wcscmp(_T("x86"), wszProcessor))
		{
			m_dwMachineType = IMAGE_FILE_MACHINE_I386;
		}
	}

	// Exception code description
	m_vErrorCodeDesc[0].m_dwErrorCode = EXCEPTION_ACCESS_VIOLATION;
	StringCchCopyA(m_vErrorCodeDesc[0].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_ACCESS_VIOLATION));
	m_vErrorCodeDesc[1].m_dwErrorCode = EXCEPTION_DATATYPE_MISALIGNMENT;
	StringCchCopyA(m_vErrorCodeDesc[1].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_DATATYPE_MISALIGNMENT));
	m_vErrorCodeDesc[2].m_dwErrorCode = EXCEPTION_BREAKPOINT;
	StringCchCopyA(m_vErrorCodeDesc[2].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_BREAKPOINT));
	m_vErrorCodeDesc[3].m_dwErrorCode = EXCEPTION_SINGLE_STEP;
	StringCchCopyA(m_vErrorCodeDesc[3].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_SINGLE_STEP));
	m_vErrorCodeDesc[4].m_dwErrorCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
	StringCchCopyA(m_vErrorCodeDesc[4].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_ARRAY_BOUNDS_EXCEEDED));
	m_vErrorCodeDesc[5].m_dwErrorCode = EXCEPTION_FLT_DENORMAL_OPERAND;
	StringCchCopyA(m_vErrorCodeDesc[5].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_DENORMAL_OPERAND));
	m_vErrorCodeDesc[6].m_dwErrorCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
	StringCchCopyA(m_vErrorCodeDesc[6].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_DIVIDE_BY_ZERO));
	m_vErrorCodeDesc[7].m_dwErrorCode = EXCEPTION_FLT_INEXACT_RESULT;
	StringCchCopyA(m_vErrorCodeDesc[7].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_INEXACT_RESULT));
	m_vErrorCodeDesc[8].m_dwErrorCode = EXCEPTION_FLT_INVALID_OPERATION;
	StringCchCopyA(m_vErrorCodeDesc[8].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_INVALID_OPERATION));
	m_vErrorCodeDesc[9].m_dwErrorCode = EXCEPTION_FLT_OVERFLOW;
	StringCchCopyA(m_vErrorCodeDesc[9].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_OVERFLOW));
	m_vErrorCodeDesc[10].m_dwErrorCode = EXCEPTION_FLT_STACK_CHECK;
	StringCchCopyA(m_vErrorCodeDesc[10].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_STACK_CHECK));
	m_vErrorCodeDesc[11].m_dwErrorCode = EXCEPTION_FLT_UNDERFLOW;
	StringCchCopyA(m_vErrorCodeDesc[11].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_FLT_UNDERFLOW));
	m_vErrorCodeDesc[12].m_dwErrorCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
	StringCchCopyA(m_vErrorCodeDesc[12].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_INT_DIVIDE_BY_ZERO));
	m_vErrorCodeDesc[13].m_dwErrorCode = EXCEPTION_INT_OVERFLOW;
	StringCchCopyA(m_vErrorCodeDesc[13].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_INT_OVERFLOW));
	m_vErrorCodeDesc[14].m_dwErrorCode = EXCEPTION_PRIV_INSTRUCTION;
	StringCchCopyA(m_vErrorCodeDesc[14].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_PRIV_INSTRUCTION));
	m_vErrorCodeDesc[15].m_dwErrorCode = EXCEPTION_IN_PAGE_ERROR;
	StringCchCopyA(m_vErrorCodeDesc[15].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_IN_PAGE_ERROR));
	m_vErrorCodeDesc[16].m_dwErrorCode = EXCEPTION_ILLEGAL_INSTRUCTION;
	StringCchCopyA(m_vErrorCodeDesc[16].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_ILLEGAL_INSTRUCTION));
	m_vErrorCodeDesc[17].m_dwErrorCode = EXCEPTION_NONCONTINUABLE_EXCEPTION;
	StringCchCopyA(m_vErrorCodeDesc[17].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_NONCONTINUABLE_EXCEPTION));
	m_vErrorCodeDesc[18].m_dwErrorCode = EXCEPTION_STACK_OVERFLOW;
	StringCchCopyA(m_vErrorCodeDesc[18].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_STACK_OVERFLOW));
	m_vErrorCodeDesc[19].m_dwErrorCode = EXCEPTION_INVALID_DISPOSITION;
	StringCchCopyA(m_vErrorCodeDesc[19].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_INVALID_DISPOSITION));
	m_vErrorCodeDesc[20].m_dwErrorCode = EXCEPTION_GUARD_PAGE;
	StringCchCopyA(m_vErrorCodeDesc[20].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_GUARD_PAGE));
	m_vErrorCodeDesc[21].m_dwErrorCode = EXCEPTION_INVALID_HANDLE;
	StringCchCopyA(m_vErrorCodeDesc[21].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_INVALID_HANDLE));
	m_vErrorCodeDesc[22].m_dwErrorCode = EXCEPTION_POSSIBLE_DEADLOCK;
	StringCchCopyA(m_vErrorCodeDesc[22].m_szErrorCode, 64, CODE_DESCR(EXCEPTION_POSSIBLE_DEADLOCK));
	m_vErrorCodeDesc[23].m_dwErrorCode = CONTROL_C_EXIT;
	StringCchCopyA(m_vErrorCodeDesc[23].m_szErrorCode, 64, CODE_DESCR(CONTROL_C_EXIT));
	// Any other exception code???

	m_pszCallStack = new char[CALLSTACK_BUFFER_SIZE];
	SecureZeroMemory(m_pszCallStack, sizeof(char) * CALLSTACK_BUFFER_SIZE);
}

StackTracer::~StackTracer(void)
{
	if (nullptr != m_pszCallStack)
	{
		delete[] m_pszCallStack;
		m_pszCallStack = nullptr;
	}
}

const char* StackTracer::GetExceptionStackTrace(const EXCEPTION_POINTERS *pExecption)
{
	// Initializes the symbol handler
	// 注意，初始化符号不要过早执行，因为过早执行，DLL可能还没有加载全
	// 如果是初始化符号之后加载的DLL崩溃，将没有机会初始化这些DLL的符号，
	// 导致解析不出模块名、函数名、行号等信息
	if (SymInitialize(m_hProcess, NULL, TRUE))
	{
		m_bSymbolInited = TRUE;
	}

	HandleException(pExecption);

	if (m_bSymbolInited)
	{
		SymCleanup(m_hProcess);
	}

	return GetExceptionMsg();
}

LONG StackTracer::ExceptionFilter(LPEXCEPTION_POINTERS e)
{
	return HandleException(e);
}

const char *StackTracer::GetExceptionMsg()
{
	SecureZeroMemory(m_pszCallStack, sizeof(char) * CALLSTACK_BUFFER_SIZE);

	// Exception Code
	StringCchPrintfA(m_szCode, 72, "0x%x", m_dwExceptionCode);

	StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "Exception Code: ");
	StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_szCode);
	StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "\r\n");

	int nError = FindErrorCodeDesc(m_dwExceptionCode);
	if (nError >= 0 && nError < ERRORCODE_DESC_COUNT)
	{
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "Exception: ");
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_vErrorCodeDesc[nError].m_szErrorCode);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "\r\n");
	}

	// Call Stack
	for (int i = 0; i < m_nCallStackCount; ++i)
	{
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE,
			(m_vCallStack[i].szModuleName[0] == 0) ? "UnknownModule" : m_vCallStack[i].szModuleName);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, " (");

		StringCchPrintfA(m_szAddrs, 128, "0x%llx", m_vCallStack[i].hModuleBaseAddr);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_szAddrs);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "-");
		StringCchPrintfA(m_szAddrs, 128, "0x%llx", m_vCallStack[i].hModuleEndAddr);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_szAddrs);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, ") ");

		StringCchPrintfA(m_szAddrs, 128, "0x%llx", m_vCallStack[i].hFuncAddress);
		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_szAddrs);

		if (m_vCallStack[i].pszFunctionName[0] != 0)
		{
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, " ");
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_vCallStack[i].pszFunctionName);
		}

		if (m_vCallStack[i].szFileName[0] != 0)
		{
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, " ");
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_vCallStack[i].szFileName);
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "[");

			StringCchPrintfA(m_szLine, 16, "%d", m_vCallStack[i].nLineNumber);
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, m_szLine);
			StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "]");
		}

		StringCchCatA(m_pszCallStack, CALLSTACK_BUFFER_SIZE, "\r\n");
	}

	return m_pszCallStack;
}

DWORD StackTracer::GetExceptionCode()
{
	return m_dwExceptionCode;
}

void StackTracer::ClearCallStack()
{
	for (size_t i = 0; i < CALLSTACK_DEPTH; i++)
	{
		m_vCallStack[i].Clear();
	}

	SecureZeroMemory(m_pszCallStack, sizeof(char) * CALLSTACK_BUFFER_SIZE);

	m_nCallStackCount = 0;
	m_dwExceptionCode = 0;
}

LONG __stdcall StackTracer::HandleException(const EXCEPTION_POINTERS *pExecption)
{
	ClearCallStack();

	m_dwExceptionCode = pExecption->ExceptionRecord->ExceptionCode;

	// Work through the call stack upwards.
	TraceCallStack(pExecption->ContextRecord);

	return(EXCEPTION_EXECUTE_HANDLER);
}

// Work through the stack to get the entire call stack
void StackTracer::TraceCallStack(CONTEXT* pContext)
{
	// Initialize stack frame
	SecureZeroMemory(&m_sf, sizeof(STACKFRAME));
	SecureZeroMemory(m_symbolBuffer, sizeof(ULONG64) * BUFFERSIZE);

#if defined(_WIN64)
	m_sf.AddrPC.Offset = pContext->Rip;
	m_sf.AddrStack.Offset = pContext->Rsp;
	m_sf.AddrFrame.Offset = pContext->Rbp;
#elif defined(WIN32)
	m_sf.AddrPC.Offset = pContext->Eip;
	m_sf.AddrStack.Offset = pContext->Esp;
	m_sf.AddrFrame.Offset = pContext->Ebp;
#endif
	m_sf.AddrPC.Mode = AddrModeFlat;
	m_sf.AddrStack.Mode = AddrModeFlat;
	m_sf.AddrFrame.Mode = AddrModeFlat;

	if (0 == m_dwMachineType)
		return;

	// Walk through the stack frames.
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	while (StackWalk64(m_dwMachineType, hProcess, hThread, &m_sf, pContext, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
	{
		if (m_sf.AddrFrame.Offset == 0 || m_nCallStackCount >= CALLSTACK_DEPTH)
			break;

		// 1. Get function name at the address
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)m_symbolBuffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		m_vCallStack[m_nCallStackCount].hFuncAddress = (HANDLE)m_sf.AddrPC.Offset;

		// Get module base address and size
		IMAGEHLP_MODULE64 ihm;
		ihm.SizeOfStruct = sizeof(ihm);
		BOOL bRet = SymGetModuleInfo(hProcess, m_sf.AddrPC.Offset, &ihm);
		if (bRet)
		{
			m_vCallStack[m_nCallStackCount].hModuleBaseAddr = (HANDLE)ihm.BaseOfImage;
			m_vCallStack[m_nCallStackCount].hModuleEndAddr = (HANDLE)(ihm.BaseOfImage + ihm.ImageSize);
			StringCchCopyA(m_vCallStack[m_nCallStackCount].szModuleName, MAX_PATH, PathFindFileNameA(ihm.ImageName));
		}

		DWORD64 dwSymDisplacement = 0;
		if (SymFromAddr(hProcess, m_sf.AddrPC.Offset, &dwSymDisplacement, pSymbol))
		{
			StringCchCopyA(m_vCallStack[m_nCallStackCount].pszFunctionName, MAX_SYM_NAME, pSymbol->Name);
		}

		//2. get line and file name at the address
		DWORD dwLineDisplacement = 0;
		SecureZeroMemory(&m_lineInfo, sizeof(IMAGEHLP_LINE64));
		m_lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		if (SymGetLineFromAddr64(hProcess, m_sf.AddrPC.Offset, &dwLineDisplacement, &m_lineInfo))
		{
			StringCchCopyA(m_vCallStack[m_nCallStackCount].szFileName, MAX_PATH, PathFindFileNameA(m_lineInfo.FileName));
			m_vCallStack[m_nCallStackCount].nLineNumber = m_lineInfo.LineNumber;
		}

		// Call stack stored
		m_nCallStackCount++;
	}
}

int StackTracer::FindErrorCodeDesc(DWORD dwError) const
{
	int nIndex = -1;
	for (int i = 0; i < ERRORCODE_DESC_COUNT; ++i)
	{
		if (m_vErrorCodeDesc[i].m_dwErrorCode == dwError)
		{
			nIndex = i;
			break;
		}
	}

	return nIndex;
}
