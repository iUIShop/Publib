#include "EnumFolder.h"

#include <strsafe.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <PathCch.h>


int IUI::EnumPath(
	LPCTSTR lpszRootPath,
	LPCTSTR lpszParentPath,
	__out LONGLONG* pllFolderCount,
	__out LONGLONG* pllFileCount,
	FuncEnumPathCallback fnCallback,
	void* pArg,
	BOOL bFullPath)
{
	if (NULL == lpszParentPath
		|| NULL == lpszRootPath)
	{
		return -1;
	}

	TCHAR szRoot[MAX_PATH] = { 0 };
	TCHAR szFind[MAX_PATH] = { 0 };
	TCHAR szRelativePath[MAX_PATH] = { 0 };
	bool bContinue = true;

	StringCchCopy(szRoot, MAX_PATH, lpszParentPath);
	PathCchAppend(szRoot, MAX_PATH, _T("*"));

	// 不能初始化为NULL
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(szRoot, &fd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return -2;
	}

	do
	{
		// 略过.和..两个文件夹
		if (StrCmp(fd.cFileName, _T(".")) == 0
			|| StrCmp(fd.cFileName, _T("..")) == 0)
		{
			continue;
		}

#ifdef _DEBUG
		// C:\Users\Administrator\AppData\Local\Application Data文件夹是个特殊的文件夹
		// Application Data中的内容，就是C:\Users\Administrator\AppData\Local中的内容
		// 如果双击Application Data文件夹进去，展示的内容与未进去之前完全相同
		// 但是路径中多了一层Application Data，一直进去，会引起死循环。
		// Application Data文件夹具有FILE_ATTRIBUTE_REPARSE_POINT属性
		if (StrCmp(fd.cFileName, _T("Application Data")) == 0)
		{
			int n = 0;
		}
#endif // _DEBUG

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// 过滤掉C:\Users\Administrator\AppData\Local\Application Data文件夹
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
			{
				// 如果是文件夹，递归遍历
				if (NULL != pllFolderCount)
				{
					(*pllFolderCount)++;
				}

				StringCchCopy(szFind, MAX_PATH, lpszParentPath);
				BOOL bRet = PathCchAppend(szFind, MAX_PATH, fd.cFileName);

				if (!bFullPath)
				{
					PathRelativePathTo(szRelativePath,
						lpszRootPath,
						FILE_ATTRIBUTE_DIRECTORY,
						szFind,
						FILE_ATTRIBUTE_NORMAL);
				}

				if (bRet)
				{
					if (nullptr != fnCallback)
					{
						bContinue = fnCallback(&fd, lpszRootPath, bFullPath ? szFind : szRelativePath, bFullPath, TRUE, pArg);
					}

					if (bContinue)
					{
						int nRet = EnumPath(lpszRootPath, szFind, pllFolderCount, pllFileCount, fnCallback, pArg, bFullPath);
						if (nRet > 0)
						{
							bContinue = false;
						}
					}
				}

				if (!bContinue)
				{
					break;
				}
			}
			else
			{
				// 文件夹为"已装好的卷"或快捷方式等类型
				if (nullptr != fnCallback)
				{
					bContinue = fnCallback(&fd, lpszRootPath, bFullPath ? szFind : szRelativePath, bFullPath, TRUE, pArg);
				}
				if (!bContinue)
				{
					break;
				}
			}
		}
		else
		{
			// 如果是文件，处理文件
			StringCchCopy(szFind, MAX_PATH, lpszParentPath);
			BOOL bRet = PathCchAppend(szFind, MAX_PATH, fd.cFileName);

			if (!bFullPath)
			{
				TCHAR szOut[MAX_PATH] = { 0 };
				PathRelativePathTo(szOut,
					lpszRootPath,
					FILE_ATTRIBUTE_DIRECTORY,
					szFind,
					FILE_ATTRIBUTE_NORMAL);
				StringCchCopy(szFind, MAX_PATH, szOut);
			}

			if (nullptr != fnCallback)
			{
				bContinue = fnCallback(&fd, lpszRootPath, szFind, bFullPath, FALSE, pArg);
			}

			if (NULL != pllFileCount)
			{
				(*pllFileCount)++;
			}

			if (!bContinue)
			{
				break;
			}
		}
	} while (FindNextFile(hFind, &fd));

	if (INVALID_HANDLE_VALUE != hFind)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}

	return 0;
}
