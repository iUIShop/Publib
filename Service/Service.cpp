// 下面两个头文件顺序不能变
#include <afxwin.h>         // MFC core and standard components
#include "Service.h"

#include <winsvc.h>
#include <strsafe.h>
#include <memory>

void AutoCloseServiceHandle(SC_HANDLE *ph)
{
	if (nullptr != ph && nullptr != *ph)
	{
		CloseServiceHandle(*ph);
		*ph = NULL;
	}
}

int IUI::InstallService(LPCTSTR lpszBinPath, DWORD* pdwError)
{
	if (nullptr == pdwError)
	{
		return -1;
	}

	SC_HANDLE hSCM = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p(&hSCM, AutoCloseServiceHandle);
	SC_HANDLE hService = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p2(&hService, AutoCloseServiceHandle);

	// Get a handle to the SCM database. 
	hSCM = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == hSCM)
	{
		*pdwError = GetLastError();
		return -2;
	}

	// Create the service.
	TCHAR szFile[MAX_PATH] = { 0 };
	StringCchCopy(szFile, MAX_PATH, lpszBinPath);
	PathStripPath(szFile);
	PathRemoveExtension(szFile);

	hService = CreateService(
		hSCM,                      // SCM database 
		szFile,                    // name of service 
		szFile,                    // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_KERNEL_DRIVER,     // service type, 如果是win32程序，为SERVICE_WIN32_OWN_PROCESS
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		lpszBinPath,               // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (hService == NULL)
	{
		*pdwError = GetLastError();
		return -3;
	}

	// 这里不能把hSCM和hService置nullptr，否则AutoCloseServiceHandle中的参数指向的对象为nullptr

	return 0;
}

int IUI::StartService2(LPCTSTR lpszServiceName, DWORD* pdwError)
{
	if (nullptr == pdwError)
	{
		return -1;
	}

	SC_HANDLE hSCM = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p(&hSCM, AutoCloseServiceHandle);
	SC_HANDLE hService = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p2(&hService, AutoCloseServiceHandle);

	//
	// 打开服务控制管理器（SCM）数据库
	//
	hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCM)
	{
		*pdwError = GetLastError();
		return -2;
	}

	//
	// 从SCM中打开服务
	//
	hService = ::OpenService(hSCM, lpszServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		*pdwError = GetLastError();
		return -3;
	}

	//
	// 启动服务
	//
	BOOL bRet = ::StartService(hService, 0, NULL);
	if (!bRet)
	{
		*pdwError = GetLastError();
		return -4;
	}

	return 0;
}

int IUI::StopService(LPCTSTR lpszServiceName, DWORD* pdwError)
{
	if (nullptr == pdwError)
	{
		return -1;
	}

	SC_HANDLE hSCM = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p(&hSCM, AutoCloseServiceHandle);
	SC_HANDLE hService = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p2(&hService, AutoCloseServiceHandle);

	//
	// 打开服务控制管理器（SCM）数据库
	//
	hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCM)
	{
		*pdwError = GetLastError();
		return -1;
	}

	//
	// 从SCM中打开服务
	//
	hService = ::OpenService(hSCM, lpszServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		*pdwError = GetLastError();
		return -2;
	}

	//
	// 停止服务
	//
	SERVICE_STATUS ss;
	BOOL bRet = ::ControlService(hService, SERVICE_CONTROL_STOP, &ss);
	if (!bRet)
	{
		*pdwError = GetLastError();
		return -3;
	}

	return 0;
}

int IUI::QueryService(LPCTSTR lpszServiceName, DWORD* pdwError, DWORD * pdwCurrentState)
{
	if (nullptr == pdwCurrentState)
	{
		return -1;
	}

	*pdwCurrentState = SERVICE_STOPPED;

	SC_HANDLE hSCM = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p(&hSCM, AutoCloseServiceHandle);
	SC_HANDLE hService = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p2(&hService, AutoCloseServiceHandle);

	//
	// 打开服务控制管理器（SCM）数据库
	//
	hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCM)
	{
		*pdwError = GetLastError();
		return -2;
	}

	//
	// 从SCM中打开服务
	//
	hService = ::OpenService(hSCM, lpszServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		*pdwError = GetLastError();
		return -3;
	}

	//
	// 查询服务
	//
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwNeed = 0;
	BOOL bRet = ::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (BYTE*)&ssp, sizeof(ssp), &dwNeed);
	if (!bRet)
	{
		*pdwError = GetLastError();
		return -4;
	}

	*pdwCurrentState = ssp.dwCurrentState;

	return 0;
}

int IUI::UninstallService(LPCTSTR lpszServiceName, DWORD* pdwError)
{
	if (nullptr == pdwError)
	{
		return -1;
	}

	SC_HANDLE hSCM = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p(&hSCM, AutoCloseServiceHandle);
	SC_HANDLE hService = NULL;
	std::unique_ptr<SC_HANDLE, decltype(AutoCloseServiceHandle)*> p2(&hService, AutoCloseServiceHandle);

	// Get a handle to the SCM database. 
	hSCM = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == hSCM)
	{
		*pdwError = GetLastError();
		return -2;
	}

	//
	// 从SCM中打开服务
	//
	hService = ::OpenService(hSCM, lpszServiceName, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		*pdwError = GetLastError();
		return -3;
	}


	// Create the service.
	BOOL bRet = DeleteService(hService);
	if (!bRet)
	{
		*pdwError = GetLastError();
		return -4;
	}

	// 这里不能把hSCM和hService置nullptr，否则AutoCloseServiceHandle中的参数指向的对象为nullptr

	return 0;
}

void IUI::FormatErrorCode(DWORD dwError, CString* pstrError)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf = nullptr;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	*pstrError = (LPCTSTR)lpMsgBuf;

	LocalFree(lpMsgBuf);
}
