#pragma once

#include <atlstr.h>
#include <list>
#include <mutex>

// 日志类
// 支持输出到文件、DebugView
// 多进程、多线程安全。

// 用法：
//CIUILog g_log; // 定义全局对象
//
//g_log.SetMaxLogFilesize(0); // 文件尺寸无上限
//TCHAR szFolder[MAX_PATH] = { 0 };
//GetRunningFolder(szFolder);
//PathAppend(szFolder, L"1.log");
//g_log.Open(szFolder);
//
//// 写日志
//LONGLONG llData = 5;
//g_log.WriteA("%I64d", llData);
//g_log.WriteA("abc");
//
//// 用完关闭
//g_log.Close();

// 注意，如果想支持%S格式化中文字符串，需要提前调用：std::locale::global(std::locale(""))
// %S是在format和字符串参数宽度不匹配的时候使用
// 例如
// printf("%S", L"aaa");
// wprintf(L"%S", "bbb");
// 即：如果是Ansi版本的printf需要格式化unicode16版本的字符串，或者unicode16版本的wprinft需要格式化ansi版本的字符串，就需要使用%S。
// Ansi版本的printf格式化ansi版本的字符串，或者unicode16版本的wprinft格式化unicode16版本的字符串，都使用%s.

#define LOG_OUTPUT_MODE_FILE	0x0001	// 输出到文件
#define LOG_OUTPUT_MODE_STDIO	0x0002	// 输出到标准IO
#define LOG_OUTPUT_MODE_DEBUGER	0x0004	// DebugView

// 进程间安全的日志类
class CIUIProcessLog
{
public:
	~CIUIProcessLog();
	CIUIProcessLog();

	int Open(LPCWSTR szFile);
	int SetMaxLogFilesize(DWORD dwMaxFilesize);
	int SetOutputMode(DWORD dwLogOutputMode);
	int WriteA(LPCSTR fmt, ...);
	int WriteW(LPCWSTR fmt, ...);
	// 如果不是可变参数，一定要用WriteString，而不是使用WriteA或WriteW
	// 防止字符串中的%被当成转义符
	int WriteStringW(LPCWSTR lpszLog);
	// 插入空行
	int InsertSpaceLine();
	BOOL Enable(BOOL bEnable);
	int Close();

private:
	CStringW m_strLogFileW;
	HANDLE m_hLogFile;
	// 进程间同步，只能有一个进程的线程写日志
	HANDLE m_hCanWriteMutex;
	BOOL m_bEnable;
	DWORD m_dwMaxFilesize;
	DWORD m_dwLogOutputMode;
};


// 生产者消费者模式的同步队列
template <typename T>
class SyncQueue
{
	bool IsFull() const
	{
		if (-1 == m_maxSize)
		{
			return false; // 队列没有限制
		}
		else
		{
			return m_queue.size() == m_maxSize;
		}
	}

	bool IsEmpty() const
	{
		return m_queue.empty();
	}

public:
	SyncQueue(int maxSize = -1) : m_maxSize(maxSize)
	{

	}

	void Put(const T& x)
	{
		// 队列如果满了，就等待。待消费线程取出数据之后发一个未满的通知，
		// 本线程被唤醒，数据被插入队列。
		// 注意，Take和Put几乎不在同一个线程中被调用。
		std::unique_lock<std::mutex> locker(m_mutex);

		// 注意：wait函数中可能会释放mutex（根据后面的条件是否返回true决定，如果返回false则释放mutex，因为条件返回false后，本线程就wait挂起了，释放了互斥，供其它线程访问），而unique_lock这里还拥有mutex。
		// 准确的来说，是本线程在调用wait时，已不再拥有mutex，但wait会阻塞本线程
		// 阻塞本线程的不是mutex，而是条件变量。
		// 虽然本线程现在不拥有mutex，但下面条件变量调用notify_one或notify_all时，
		// 会再次拥有mutex，所以，不影响unique_lock释放mutex。
		// wait返回的条件是别的线程调用了notify_all或notify_one、且后面的条件为true。两者缺一不可。
		// 通俗理解下面这句话的意思是：
		// 如果队列满了，就释放互斥，让其它线程有机会拿到互斥后从队列取走数据，且挂起线程，等待队列不满后插入数据。
		// 如果队列不满，就继续拥有互斥，且wait立马返回后，把数据插入队列。调用不空的条件变量，通知Take线程取数据。
		m_notFull.wait(locker, [this] {return !IsFull(); }); // 当后面的条件返回false时，会释放mutex，返回true不会释放，且wait函数马上返回。

		m_queue.push_back(x);
		m_notEmpty.notify_one(); // 再次拥有mutex
	} // unique_lock释放mutex

	void Take(T& x)
	{
		// 如果队列为空，就不能取数据，线程将等待，等待插入数据的线程发出不为空的通知时，
		// 本线程被唤醒，数据被取走。
		// 注意，Take和Put几乎不在同一个线程中被调用。
		std::unique_lock<std::mutex> locker(m_mutex);
		m_notEmpty.wait(locker, [this] {return !IsEmpty(); });

		x = m_queue.front();
		m_queue.pop_front();
		m_notFull.notify_one();
	}

	bool Empty()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.empty();
	}

	bool Full()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size() == m_maxSize;
	}

	size_t Size()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size();
	}

private:
	// 一个队列
	std::list<T> m_queue;
	// 注意：两个条件变量，一个互斥量。因为有两个条件，一个是判断队列是否为空，另一个是判断是否队列已满
	// 但保护的是同一个队列，所以只需要一个互斥。
	std::mutex m_mutex;
	std::condition_variable m_notEmpty;
	std::condition_variable m_notFull;
	int m_maxSize = -1;	// 表示队列的长度没有限制
};

// 线程间安全的日志类
class CIUILog
{
public:
	~CIUILog();
	CIUILog();

	int Open(LPCWSTR szFile);
	int SetMaxLogFilesize(DWORD dwMaxFilesize);
	int SetOutputMode(DWORD dwLogOutputMode);
	int WriteA(LPCSTR fmt, ...);
	int WriteW(LPCWSTR fmt, ...);
	// 如果不是可变参数，一定要用WriteString，而不是使用WriteA或WriteW
	// 防止字符串中的%被当成转义符
	int WriteStringW(LPCWSTR lpszLog);
	// 插入空行
	int InsertSpaceLine();
	BOOL Enable(BOOL bEnable);
	int Close();

protected:
	int WriteThread();

private:
	CStringW m_strLogFileW;
	HANDLE m_hLogFile;
	BOOL m_bEnable;
	DWORD m_dwMaxFilesize;
	DWORD m_dwLogOutputMode;

	SyncQueue<CStringW> m_qLogs; // 线程间安全的队列
	std::thread m_threadWirte;	// 写日志线程
	std::atomic<bool> m_bThreadExit = false; // 线程退出标志
};


int GetRunningFolder(TCHAR szFolder[MAX_PATH]);
void write_log(const WCHAR* format, ...);
