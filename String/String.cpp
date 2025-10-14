#include "String.h"
#include <vector>
#include <atlstr.h>
#include <strsafe.h>
#include <atlenc.h>


std::wstring IUI::AnsiToUnicode(const char* pszSrc)
{
	// 返回字符数，如果第四个参数为-1，返回的字符数包含“终止空字符”，否则不含。
	// 最后一个参数为指向倒数第二个参数的字符串的大小（以字符为单位），为零时，返回需要的
	// 目标字符串大小（以字符为单位）。
	// 例如：MultiByteToWideChar(CP_ACP, 0, "abc", -1, NULL, 0)返回4。
	// MultiByteToWideChar(CP_ACP, 0, "abcdefg", 2, NULL, 0);返回2，这是因为第四个参数指定了
	// 要转换的字符个数，由于指定的个数不包含终止空字符，所以返回的需要的字符串空间，也不包含终止空字符。
	// 为了安全起见，在分配目标字符串空间的时候，多分配一些，例如，msdn上多分配了10个字符。
	int nNeedLen = MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, NULL, 0);
	if (nNeedLen <= 0)
	{
		return L"";
	}

	std::vector<WCHAR> pszUnicode16(nNeedLen + 10, 0);

	nNeedLen = MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, &pszUnicode16[0], nNeedLen);
	if (nNeedLen <= 0)
	{
		return L"";
	}

	return &pszUnicode16[0];
}

std::string IUI::UnicodeToAnsi(const WCHAR* pszSrc)
{
	int nNeedLen = WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, NULL, 0, NULL, NULL);
	if (nNeedLen <= 0)
	{
		return "";
	}

	std::vector<CHAR> pszAnsi(nNeedLen + 10, 0);

	BOOL bFailed = TRUE;
	WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, &pszAnsi[0], nNeedLen, NULL, &bFailed);
	if (bFailed)
	{
		OutputDebugStringW(L"有部分字符未能成功从宽字符转成多字节。\r\n");
	}

	return &pszAnsi[0];
}

std::string IUI::UnicodeToUTF8(const WCHAR* pszSrcUnicode16)
{
	int nNeedLen = WideCharToMultiByte(CP_UTF8, 0, pszSrcUnicode16, -1, NULL, 0, NULL, NULL);
	if (nNeedLen <= 0)
	{
		return "";
	}

	std::vector<CHAR> pszUtf8(nNeedLen + 10, 0);

	// 对于UTF7或UTF8，最后一个参数必须为NULL。见MSDN
	int nWriteByte = WideCharToMultiByte(CP_UTF8, 0, pszSrcUnicode16, -1, &pszUtf8[0], nNeedLen, NULL, NULL);

	return &pszUtf8[0];
}

// 与AnsiToUnicode一样，只是把MultiByteToWideChar的第一个参数，由CP_ACP换成了CP_UTF8
std::wstring IUI::UTF8ToUnicode(const char* pszSrcUTF8)
{
	int nNeedLen = MultiByteToWideChar(CP_UTF8, 0, pszSrcUTF8, -1, NULL, 0);
	if (nNeedLen <= 0)
	{
		return L"";
	}

	std::vector<WCHAR> pszUnicode16(nNeedLen + 10, 0);

	nNeedLen = MultiByteToWideChar(CP_UTF8, 0, pszSrcUTF8, -1, &pszUnicode16[0], nNeedLen);
	if (nNeedLen <= 0)
	{
		return L"";
	}

	return &pszUnicode16[0];
}

std::string IUI::AnsiToUTF8(const CHAR* pszSrc)
{
	std::wstring strUtf16 = AnsiToUnicode(pszSrc);
	return UnicodeToUTF8(strUtf16.c_str());
}

std::string IUI::UTF8ToAnsi(const CHAR* pszSrcUTF8)
{
	std::wstring strUtf16 = UTF8ToUnicode(pszSrcUTF8);
	return UnicodeToAnsi(strUtf16.c_str());
}

#ifndef _strinc
#define _strinc(_pc)    ((_pc)+1)
#endif

std::string IUI::TrimLeft(const char* pszSrc)
{
	const char* lpsz = pszSrc;
	const char* lpszFirst = NULL;

	while (*lpsz != '\0')
	{
		if (!isspace(*lpsz))
		{
			lpszFirst = lpsz;
			break;
		}

		lpsz = _strinc(lpsz);
	}

	std::string strRet;
	if (lpszFirst != NULL)
	{
		strRet = lpszFirst;
	}

	return strRet;
}

std::string IUI::TrimRight(const char* pszSrc)
{
	const char* lpsz = pszSrc;
	const char* lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (isspace(*lpsz))
		{
			if (lpszLast == NULL)
			{
				lpszLast = lpsz;
			}
		}
		else
		{
			lpszLast = NULL;
		}
		lpsz = _strinc(lpsz);
	}

	std::string strRet;
	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		strRet.assign(pszSrc, lpszLast);
	}
	else
	{
		strRet = pszSrc;
	}

	return strRet;
}

int IUI::FormatNumberWithCommasA(ULONGLONG number, std::string* pstrNumber)
{
	if (nullptr == pstrNumber)
	{
		return -1;
	}

	CStringA strNumber;
	strNumber.Format("%I64u", number);

	int length = strNumber.GetLength();
	int numCommas = (length - 1) / 3;

	for (int i = 1; i <= numCommas; i++)
	{
		int pos = length - (i * 3);
		strNumber.Insert(pos, ",");
	}

	*pstrNumber = strNumber;

	return 0;
}

int IUI::FormatNumberWithCommasW(ULONGLONG number, std::wstring* pstrNumber)
{
	if (nullptr == pstrNumber)
	{
		return -1;
	}

	CStringW strNumber;
	strNumber.Format(L"%I64u", number);

	int length = strNumber.GetLength();
	int numCommas = (length - 1) / 3;

	for (int i = 1; i <= numCommas; i++)
	{
		int pos = length - (i * 3);
		strNumber.Insert(pos, L",");
	}

	*pstrNumber = strNumber;

	return 0;
}

std::wstring IUI::GenerateGUID()
{
	GUID guid;
	HRESULT hCreateGuid = CoCreateGuid(&guid);

	if (hCreateGuid != S_OK)
	{
		return L"";
	}

	WCHAR wszGuid[40] = { 0 };
	int nChars = StringFromGUID2(guid, wszGuid, 39);

	return wszGuid;
}

int LPCWSTRToSAFEARRAY(LPCWSTR lpwszSource, SAFEARRAY** ppsaDestination)
{
	if (!lpwszSource || !ppsaDestination)
	{
		return E_INVALIDARG;
	}

	// 计算字符串长度，不包括终止的null字符
	size_t cchLength = wcslen(lpwszSource);

	// 创建一个一维的SAFEARRAY，元素类型为VT_UI2（16位无符号整数） 
	*ppsaDestination = ::SafeArrayCreateVector(VT_UI2, 0, (ULONG)cchLength);
	if (nullptr == *ppsaDestination)
	{
		return -2;
	}

	// 将字符串复制到SAFEARRAY
	for (ULONG i = 0; i < cchLength; ++i)
	{
		USHORT usElement = (USHORT)lpwszSource[i];
		//hr = ::SafeArrayPutElement(*ppsaDestination, &i, &usElement);
		//if (FAILED(hr)) {
		//	// 如果失败，需要释放SAFEARRAY
		//	::SafeArrayDestroy(*ppsaDestination);
		//	*ppsaDestination = NULL;
		//	return hr;
		//}
	}

	return S_OK;
}

// Use SysFreeString free
BSTR IUI::WCHARToBSTR(const WCHAR* pszSrc)
{
	return SysAllocString(pszSrc);
}

std::string IUI::Base64Encode(const BYTE* pbSrcData, int cbSrcLen)
{
	std::vector<char> vBase64Text;
	int nRequiredLength = Base64EncodeGetRequiredLength(cbSrcLen, 0);
	vBase64Text.resize(nRequiredLength, 0);

	BOOL bRet = ATL::Base64Encode(pbSrcData, cbSrcLen, &vBase64Text[0], &nRequiredLength);

	std::string strBase64Text;
	if (bRet)
	{
		strBase64Text.assign(&vBase64Text[0], nRequiredLength);
	}

	return strBase64Text;
}

std::string IUI::Base64Encode(const char* pUtf8)
{
	int nLen = (int)strlen(pUtf8);
	std::vector<char> vBase64Text;
	vBase64Text.resize(nLen * 2, 0);
	int nTargetLen = nLen * 2 - 1;
	BOOL bRet = ATL::Base64Encode((const BYTE*)pUtf8, nLen, &vBase64Text[0], &nTargetLen);

	std::string strBase64Text;
	if (bRet)
	{
		strBase64Text.assign(&vBase64Text[0], nTargetLen);
	}

	return strBase64Text;
}

std::string IUI::Base64Decode(const char* pszBase64)
{
	int nLenDecode = (int)strlen(pszBase64);
	std::vector<BYTE> vNormalText;
	vNormalText.resize(nLenDecode);
	BOOL bRet2 = ATL::Base64Decode(pszBase64, nLenDecode, &vNormalText[0], &nLenDecode);
	std::string strUtf8((const char*)&vNormalText[0], nLenDecode);

	return strUtf8;
}
