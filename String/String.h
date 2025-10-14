#pragma once

#include <windows.h>
#include <string>

// 使用CString作为参数和返回值，经常会碰到链接不过的情况。
namespace IUI
{
	std::wstring AnsiToUnicode(const char* pszSrc);
	std::string UnicodeToAnsi(const WCHAR* pszSrc);
	std::string UnicodeToUTF8(const WCHAR* pszSrcUnicode16);
	std::wstring UTF8ToUnicode(const char* pszSrcUTF8);
	std::string AnsiToUTF8(const CHAR* pszSrc);
	std::string UTF8ToAnsi(const CHAR* pszSrcUTF8);

	std::string TrimLeft(const char* pszSrc);
	std::string TrimRight(const char* pszSrc);

	// 把数字1234567格式化为：1,234,567
	int FormatNumberWithCommasA(ULONGLONG number, std::string* pstrNumber);
	int FormatNumberWithCommasW(ULONGLONG number, std::wstring* pstrNumber);

	std::wstring GenerateGUID();

	// Use SysFreeString free
	BSTR WCHARToBSTR(const WCHAR* pszSrc);

	// 如果源是包含中文的字符串，请使用utf8编码。
	std::string Base64Encode(const BYTE* pbSrcData, int cbSrcLen);
	std::string Base64Encode(const char* pUtf8);
	std::string Base64Decode(const char* pszBase64);
}
