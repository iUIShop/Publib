#pragma once

#include <windows.h>
#include <DbgHelp.h>


// 用途：结构化或向量异常捕获后，用来解析出调用栈
// 注意：使用GetExceptionStackTrace返回调用栈后，EXCEPTION_POINTERS 参数中的值可能会
// 改变，如果用这个参数生成dump，则生成的dump，有可能无法定位到具体代码行。
// 但先生成dump，再通过EXCEPTION_POINTERS调用GetExceptionStackTrace生成调用栈，则dump和
// 调用栈都正常。
// 捕获异常时，内存、栈可能都已破环，所以在异常回调中，尽量不要申请资源
// 所以，在StackTracer构造时，要提前申请好资源
// 用法：定义全局变量：
//	std::mutex g_mutexCallStack;
// 	StackTracer g_tracer; // 定义成全局变量可以提前分配需要的资源
// 	void* g_pVectorExceptionHandle = nullptr;
// 在程序入口调用：
//	g_pVectorExceptionHandle = AddVectoredExceptionHandler(TRUE, VectoredExceptionHandler);
// 异常回调的实现：
//LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
//{
//	// filter DBG_PRINTEXCEPTION_C, DBG_PRINTEXCEPTION_WIDE_C exception, its throw by OutputDebugString
//	if (DBG_PRINTEXCEPTION_C == pExceptionInfo->ExceptionRecord->ExceptionCode
//		|| /*DBG_PRINTEXCEPTION_WIDE_C*/0x4001000A == pExceptionInfo->ExceptionRecord->ExceptionCode)
//	{
//		return EXCEPTION_CONTINUE_SEARCH;
//	}
//
//	LONG lr = EXCEPTION_CONTINUE_SEARCH;
//
//	if (EXCEPTION_ACCESS_VIOLATION == pExceptionInfo->ExceptionRecord->ExceptionCode)
//	{
//		// EXCEPTION_ACCESS_VIOLATION
//		// such as: char *p = nullptr; *p = 1;
//		// can raise exception repeated.
//		// so, Remove Vectored Exception Handler.
//		RemoveVectoredExceptionHandler(g_pVectorExceptionHandle);
//	}
//
//	// 这6个异常是VS2022异常设置窗口中，win32异常默认项，但DBG_CONTROL_C是按键Ctrl+C触发的异常，
//	// DBG_CONTROL_BREAK是按键Ctrl+Break触发的异常，这里不需要处理
//	if (// DBG_CONTROL_C/*0x40010005L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
//		//|| DBG_CONTROL_BREAK/*0x40010008L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
//		EXCEPTION_ACCESS_VIOLATION/*0xC0000005L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
//		|| EXCEPTION_INVALID_HANDLE/*0xC0000008L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
//		|| STATUS_ASSERTION_FAILURE/*0xC0000420L*/ == pExceptionInfo->ExceptionRecord->ExceptionCode
//		|| 0xE073616E /*Sanitizer error detected */ == pExceptionInfo->ExceptionRecord->ExceptionCode)
//	{
//		// DbgHelp.h中的好多API都是非线程安全的，这里加个锁。
//		std::lock_guard<std::mutex> locker(g_mutexCallStack);
//
//		const char * pszCallStack = g_tracer.GetExceptionStackTrace(pExceptionInfo);
//	}
//
//	return lr;
//}
// 代码来源：https://www.cnblogs.com/bodong/p/12565151.html

#define CALLSTACK_DEPTH			24
#define STACKFRAME_LENGTH		1024
#define CALLSTACK_BUFFER_SIZE	(STACKFRAME_LENGTH * CALLSTACK_DEPTH)
#define ERRORCODE_DESC_COUNT	24

struct StackFrame
{
	StackFrame();
	~StackFrame();
	void Clear();

	HANDLE hFuncAddress = nullptr;
	HANDLE hModuleBaseAddr = nullptr;
	HANDLE hModuleEndAddr = nullptr;
	CHAR szModuleName[MAX_PATH] = { 0 };
	CHAR *pszFunctionName = nullptr;
	CHAR szFileName[MAX_PATH] = { 0 };
	int nLineNumber = 0;
};

struct CErrorCodeDesc
{
	DWORD m_dwErrorCode = 0;
	CHAR m_szErrorCode[64] = { 0 };
};

#define BUFFERSIZE (sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)

class StackTracer
{
public:
	StackTracer(void);
	~StackTracer(void);

	const char* GetExceptionStackTrace(const EXCEPTION_POINTERS *pExecption);

private:
	// Always return EXCEPTION_EXECUTE_HANDLER after getting the call stack
	LONG ExceptionFilter(LPEXCEPTION_POINTERS e);

	// return the exception message along with call stacks
	const char* GetExceptionMsg();

	// Return exception code and call stack data structure so that 
	// user could customize their own message format
	DWORD GetExceptionCode();

	void ClearCallStack();

private:
	// The main function to handle exception
	LONG __stdcall HandleException(const EXCEPTION_POINTERS *pExecption);

	// Work through the stack upwards to get the entire call stack
	void TraceCallStack(CONTEXT* pContext);

	int FindErrorCodeDesc(DWORD dwError) const;

private:
	DWORD m_dwExceptionCode;

	StackFrame m_vCallStack[CALLSTACK_DEPTH];
	int m_nCallStackCount = 0;

	CErrorCodeDesc m_vErrorCodeDesc[ERRORCODE_DESC_COUNT];

	DWORD m_dwMachineType; // Machine type matters when trace the call stack (StackWalk64)

	char* m_pszCallStack = nullptr;

	// temp buffer
	char m_szCode[72] = { 0 };
	char m_szAddrs[128] = { 0 };
	char m_szLine[16] = { 0 };
	char m_szModuleName[MAX_PATH] = { 0 };
	STACKFRAME64 m_sf;
	ULONG64 m_symbolBuffer[BUFFERSIZE] = { 0 };
	IMAGEHLP_LINE64 m_lineInfo = { sizeof(IMAGEHLP_LINE64) };
	HANDLE m_hProcess = GetCurrentProcess();
	BOOL m_bSymbolInited = FALSE;
};
