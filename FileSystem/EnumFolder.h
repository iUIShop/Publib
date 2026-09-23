#pragma once

#include <windows.h>

namespace IUI
{
	typedef bool(*FuncEnumPathCallback)(const WIN32_FIND_DATA *pfd, LPCTSTR lpszRoot, LPCTSTR lpszPath, BOOL bFullPath, BOOL bDir, void* pArg);

	// lpszRootPath: 待遍历的文件夹. 如果bFullPath为FALSE，基于lpszRootPath求相对路径。
	// lpszParentPath: 递归时，待遍历的(子)文件夹，在用户初次调用时，传入与lpszRootPath相同的路径.
	// pllFolderCount: 返回遍历完后得到文件夹数量。如果不需要，传入NULL.
	// pllFileCount: 返回遍历完后得到文件数量。如果不需要，传入NULL.
	// fnCallback: 调用者指定的遍历回调函数。当遍历到文件(夹)后，就会回调它。
	// pArg: 调用者指定的回调函数参数。通过FuncEnumPathCallback的最后一个参数回传给调用者。
	// bFullPath: 传给回调函数的路径是否是完整路径。如果为FALSE，则为相对路径。
	int EnumPath(
		LPCTSTR lpszRootPath,
		LPCTSTR lpszParentPath,
		__out LONGLONG* pllFolderCount,
		__out LONGLONG* pllFileCount,
		FuncEnumPathCallback fnCallback,
		void* pArg,
		BOOL bFullPath);

}

//示例：
/*********

bool EnumPathCallback(const WIN32_FIND_DATA *pfd, LPCTSTR lpszRoot, LPCTSTR lpszPath, BOOL bFullPath, BOOL bDir, void* pArg)
{
	std::list<std::wstring>* plstPath = (std::list<std::wstring> *)pArg;
	if (NULL != plstPath)
	{
		plstPath->push_back(lpszPath);
	}

	return true;
}

void CRemoveDuplicateFilesDlg::OnBnClickedBtnBegin()
{
	LONGLONG llFolderCount = 0;
	LONGLONG llFileCount = 0;
	void* p = NULL;
	std::list<std::wstring> lstPath;
	Wow64DisableWow64FsRedirection(&p);	// 32位程序扫描64位电脑，需要禁止文件重定向，否则有些文件会扫不出来。
	IUI::EnumPath(_T("E:\\Temp"), _T("E:\\Temp"), &llFolderCount, &llFileCount, EnumPathCallback, &lstPath, TRUE);
	Wow64RevertWow64FsRedirection(p);
}

*********/
