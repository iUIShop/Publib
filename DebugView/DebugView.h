#pragma once

#include <windows.h>
#include <functional>
#include <process.h>
#include <thread>

namespace IUI
{
	static const int buffer_size = 4096; // fixed, don't change
	struct DbgViewBuffer
	{
		DWORD pid;
		char str[buffer_size - sizeof(DWORD)];
	};

	// DebugView对象必须定义为static或new出来，因为线程函数会使用对象中的成员变量。
	class DebugView
	{
		typedef std::function<void(DWORD pid, const char* str)> OnNotify;

	public:
		DebugView();
		~DebugView();

	public:
		// 当收到OutputDebugString后，调用notify
		int Init(OnNotify notify);

		void Uninit();

	protected:
		unsigned int ThreadProc();

	protected:
		std::thread m_t;				  // 接收线程
		HANDLE m_hFile = nullptr;         // 共享内存
		HANDLE m_hEvtBufReady = nullptr;  // 共享内存闲置
		HANDLE m_hEvtDataReady = nullptr; // 数据备妥
		HANDLE m_hEvtExit = nullptr;      // 退出事件
		DbgViewBuffer* m_pViewOfFile = nullptr;  // 共享内存映射

		OnNotify m_notify;
	};
}
