#pragma once

#include <windows.h>
#include <string>

// 计算CRC32
namespace IUI
{
	enum CRC32_MSG
	{
		// wParam: 哪个函数发生的错误, 类型：const WCHAR *; lParam: MAKELPARAM(CRC32函数序号, error code)
		CM_ERROR = 1,
		// lParam: 总长度
		CM_BEGIN,
		// wParam: 当前已处理长度; lParam: 总长度
		CM_PROGRESS,
		// lParam: 计算出来的CRC32字符串指针，类型：const WCHAR *.
		CM_END
	};

	typedef int (*CRC32Proc)(CRC32_MSG uMsg, WPARAM wParam, LPARAM lParam, void* pUserData);

	// 同步，计算进度通过OnCRC32Proc传回，计算结果即可以通过OnCRC32Proc传回，也可以通过pstrHashResult传回。
	// pUserData: 用户自定义的OnCRC32Proc参数，通过OnCRC32Proc最后一个参数传递。
	int GetDataCRC32(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData);

	// 同步，计算进度通过OnCRC32Proc传回，计算结果即可以通过OnCRC32Proc传回，也可以通过pstrHashResult传回。
	// 当GetFileCRC32作为std::thread线程函数的时候，为了方便传参，把文件路径参数设计成const wstring&类型，而不是LPCWSTR类型，因为LPCWSTR是浅拷贝。
	int GetFileCRC32(const std::wstring &strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData);

	// 异步，计算进度和结果通过OnCRC32Proc传回
	int GetFileCRC32Async(const std::wstring& strFilePath, BOOL bUpperCase, CRC32Proc OnCRC32Proc, void* pUserData);
}
