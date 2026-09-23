#pragma once


namespace IUI
{
	int InstallService(LPCTSTR lpszBinPath, DWORD *pdwError);
	int StartService2(LPCTSTR lpszServiceName, DWORD* pdwError);
	int StopService(LPCTSTR lpszServiceName, DWORD* pdwError);
	int QueryService(LPCTSTR lpszServiceName, DWORD* pdwError, DWORD* pdwCurrentState);
	int UninstallService(LPCTSTR lpszServiceName, DWORD* pdwError);

	void FormatErrorCode(DWORD dwError, CString* pstrError);
}
