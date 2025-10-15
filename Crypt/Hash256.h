#pragma once

#include <windows.h>
#include <string>

// 计算Hash256
namespace IUI
{
	enum HASH256_MSG
	{
		// wParam: 哪个函数发生的错误, 类型：const WCHAR *; lParam: MAKELPARAM(Hash256函数序号, error code)
		HM_ERROR = 1,
		// lParam: 总长度
		HM_BEGIN,
		// wParam: 当前已处理长度; lParam: 总长度。单位size_t
		HM_PROGRESS,
		// lParam: 计算出来的Hash256字符串指针，类型：const WCHAR *.
		HM_END
	};

	typedef int (*Hash256Proc)(HASH256_MSG uMsg, WPARAM wParam, LPARAM lParam, void* pUserData);

	// 同步，计算进度通过OnHash256Proc传回，计算结果即可以通过OnHash256Proc传回，也可以通过pstrHashResult传回。
	// pUserData: 用户自定义的OnHash256Proc参数，通过OnHash256Proc最后一个参数传递。
	int GetDataHash256(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData);

	// 同步，计算进度通过OnHash256Proc传回，计算结果即可以通过OnHash256Proc传回，也可以通过pstrHashResult传回。
	// 当GetFileHash256作为std::thread线程函数的时候，为了方便传参，把文件路径参数设计成const wstring&类型，而不是LPCWSTR类型，因为LPCWSTR是浅拷贝。
	int GetFileHash256(const std::wstring &strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData);

	// 异步，计算进度和结果通过OnHash256Proc传回
	int GetFileHash256Async(const std::wstring& strFilePath, BOOL bUpperCase, Hash256Proc OnHash256Proc, void* pUserData);
}