#include "Reg.h"
#include <comutil.h>
#include <atlstr.h>
#include <Shlwapi.h>

#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif
#ifndef RRF_SUBKEY_WOW6432KEY
#define RRF_SUBKEY_WOW6432KEY 0x00020000
#endif


int SetRegDataW(
	HKEY hRootKey,
	LPCWSTR lpszSubKey,
	LPCWSTR lpszValue,
	DWORD dwType,
	const BYTE *pData,
	DWORD cbData,
	IUI::REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;
	HKEY hKey = NULL;

	do
	{
		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:// 32位程序访问32位注册表视图，64位程序访问64位注册表视图
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY; // 显式访问64位注册表视图
			break;
		default:
			break;
		}
		LSTATUS lRet = RegOpenKeyExW(hRootKey, lpszSubKey, 0, samDesired, &hKey);
		if (ERROR_FILE_NOT_FOUND == lRet)
		{
			// 如果子键不存在，创建出所有子键。 RegCreateKeyEx一次可以创建出多级注册表子键。
			DWORD dwDisposition = 0;
			lRet = RegCreateKeyExW(hRootKey,
				lpszSubKey,
				0,
				NULL,
				0,
				samDesired,
				NULL,
				&hKey,
				&dwDisposition);
		}
		if (ERROR_SUCCESS != lRet || NULL == hKey)
		{
			nRet = -2;
			break;
		}

		// 注：RegSetValue仅能修改子键的默认“值”。修改非默认“值”，要使用RegSetValueEx
		lRet = RegSetValueExW(
			hKey,
			lpszValue,
			0,
			dwType,
			pData,
			cbData);
		if (ERROR_SUCCESS != lRet)
		{
			break;
		}
	} while (false);

	if (NULL != hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

int GetRegDataW(
	HKEY hRootKey,
	LPCWSTR lpszSubKey,
	LPCWSTR lpszValue,
	DWORD dwType,
	__out std::vector<BYTE> *pvData,
	IUI::REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;
	HKEY hKey = NULL;
	LPBYTE lpData = NULL;
	DWORD cbData = 0;

	do
	{
		if (NULL == hRootKey || NULL == lpszSubKey || NULL == lpszValue || NULL == pvData)
		{
			nRet = -1;
			break;
		}

		// 正常情况下，不需要调用RegOpenKeyEx，直接调用RegGetValue就可以读取数据
		// 但在32位程序读64位注册表值时，例如：HKEY_LOCAL_MACHINE\SOFTWARE\下的键时，
		// 即使加上RRF_SUBKEY_WOW6464KEY标记，也读取不到。所以，先用RegOpenKeyEx带
		// KEY_WOW64_64KEY参数打开Key.
		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY;
			break;
		default:
			break;
		}

		LSTATUS lRet = RegOpenKeyExW(hRootKey, lpszSubKey, 0, samDesired, &hKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}

		DWORD dwFlags = 0;
		switch (dwType)
		{
		case REG_SZ:
			dwFlags |= RRF_RT_REG_SZ;
			break;

		case REG_EXPAND_SZ:
			// 在RRF_RT_REG_EXPAND_SZ上按F12，定位到RRF_RT_REG_EXPAND_SZ的定义处，注释显示要加上RRF_NOEXPAND，否则RegGetValueW会失败
			dwFlags |= (RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND);
			break;

		case REG_BINARY:
			dwFlags |= RRF_RT_REG_BINARY;
			break;

		case REG_DWORD:
			dwFlags |= RRF_RT_REG_DWORD;
			break;

		//case REG_DWORD_LITTLE_ENDIAN:
		case REG_DWORD_BIG_ENDIAN:
		case REG_LINK:
		case REG_RESOURCE_LIST:
		case REG_FULL_RESOURCE_DESCRIPTOR:
		case REG_RESOURCE_REQUIREMENTS_LIST:
		//case REG_QWORD_LITTLE_ENDIAN:
			_ASSERT(FALSE);
			break;

		case REG_MULTI_SZ:
			dwFlags |= RRF_RT_REG_MULTI_SZ;
			break;

		case REG_QWORD:
			dwFlags |= RRF_RT_REG_QWORD;
			break;

		default:
			break;
		}
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			dwFlags |= RRF_SUBKEY_WOW6432KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			dwFlags |= RRF_SUBKEY_WOW6464KEY;
			break;
		default:
			break;
		}

		// 得到需要的buf长度
		lRet = RegGetValueW(
			hKey,
			NULL,
			lpszValue,
			dwFlags,
			&dwType,
			NULL,
			&cbData);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -3;
			break;
		}

		// 虽然MSDN上说RegGetValue返回的cbData是所需要的空间
		// 但运行中碰到过如果正好分配这些空间，再次调用RegGetValue
		// 时仍然返回空间不足的情况（Win7上得到Redis路径时碰到：
		// 	std::string strRedisPath = GetRegString(HKEY_LOCAL_MACHINE,
		//		"SYSTEM\\ControlSet002\\services\\Redis",
		//		"ImagePath");）
		// 所以这里加2个额外字节。
		// 见MSDN: RegGetValueA如果查询的是REG_SZ、REG_MULTI_SZ或REG_EXPAND_SZ类型，
		// 并且使用此函数的 ANSI 版本（通过显式调用 RegGetValueA 或在包含 Windows.h 文件之前未定义 UNICODE），
		// 则此函数会将存储的 Unicode 字符串转换为 ANSI 字符串，然后再将其复制到 pvData 指向的缓冲区。
		// 并且返回的cbData是包含结束符的
		cbData += 2;
		lpData = new BYTE[cbData];
		memset(lpData, 0, cbData);
		lRet = RegGetValueW(
			hKey,
			NULL,
			lpszValue,
			dwFlags,
			&dwType,
			lpData,
			&cbData);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -4;
			break;
		}

		pvData->assign(lpData, lpData + cbData);
	} while (false);

	if (NULL != lpData)
	{
		delete[]lpData;
		lpData = NULL;
	}
	if (NULL != hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

int IUI::FormatRetrunValue(LSTATUS lRet)
{
	if (ERROR_SUCCESS != lRet)
	{
		LPWSTR lpszMsg = nullptr;
		DWORD result = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lRet,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpszMsg,
			0,
			NULL);
		if (result != 0 && lpszMsg != nullptr)
		{
			LocalFree(lpszMsg);
			lpszMsg = nullptr;
		}
	}

	return 0;
}

// lpszSubKey不能以L"\\"开头，但结尾是否包含L"\\"不受影响。L"\\"不能用L"/"代替
int IUI::CreateRegKeyW(HKEY hRootKey, LPCWSTR lpszSubKey, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	HKEY hKey = NULL;
	int nRet = 0;

	do
	{
		if (nullptr == hRootKey || nullptr == lpszSubKey)
		{
			nRet = -1;
			break;
		}

		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:// 32位程序访问32位注册表视图，64位程序访问64位注册表视图
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY; // 显式访问64位注册表视图
			break;
		default:
			break;
		}

		// RegCreateKeyEx一次可以创建出多级注册表子键。
		DWORD dwDisposition = 0;
		LSTATUS lRet = RegCreateKeyExW(hRootKey,
			lpszSubKey,
			0,
			NULL,
			0,
			samDesired,
			NULL,
			&hKey,
			&dwDisposition);
		FormatRetrunValue(lRet);

		if (ERROR_SUCCESS != lRet || NULL == hKey)
		{
			nRet = -2;
			break;
		}
	} while (FALSE);

	if (NULL != hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

int IUI::SetRegDwordW(
	HKEY hRootKey,
	LPCWSTR lpszSubKey,
	LPCWSTR lpszValue,
	DWORD dwData,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	return SetRegDataW(hRootKey, lpszSubKey, lpszValue, REG_DWORD, (const BYTE *)&dwData, sizeof(DWORD), eRaf);
}

int IUI::GetRegDwordW(HKEY hRootKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue, __out DWORD *pdwRet, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	if (NULL == pdwRet)
	{
		return -1;
	}

	std::vector<BYTE> vDword;
	int nRet = GetRegDataW(hRootKey, lpszSubKey, lpszValue, REG_DWORD, &vDword, eRaf);
	if (0 != nRet)
	{
		return nRet;
	}

	if (vDword.size() != sizeof(DWORD))
	{
		_ASSERT(FALSE);
		return -2;
	}

	memcpy(pdwRet, &vDword[0], sizeof(DWORD));
	return 0;
}

int IUI::SetRegBinaryW(
	HKEY hRootKey,
	LPCWSTR lpszSubKey,
	LPCWSTR lpszValue,
	const BYTE *pData,
	DWORD cbData,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	return SetRegDataW(hRootKey, lpszSubKey, lpszValue, REG_BINARY, pData, cbData, eRaf);
}

int IUI::GetRegBinaryW(
	HKEY hRootKey,
	LPCWSTR lpszSubKey,
	LPCWSTR lpszValue,
	__out std::vector<BYTE> *pvData,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	return GetRegDataW(hRootKey, lpszSubKey, lpszValue, REG_BINARY, pvData, eRaf);
}

int IUI::SetRegStringA(
	HKEY hRootKey,
	const char *pszSubKey,
	const char *pszValue,
	const char *pszData,
	REG_VALUE_STRING_TYPE eStringType/* = RVST_SZ*/,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	return SetRegStringW(hRootKey,
		(LPCWSTR)_bstr_t(pszSubKey),
		(LPCWSTR)_bstr_t(pszValue),
		(LPCWSTR)_bstr_t(pszData),
		eStringType,
		eRaf);
}

int IUI::GetRegStringA(
	HKEY hRootKey,
	const char *pszSubKey,
	const char *pszValue,
	std::string *pstrRet,
	REG_VALUE_STRING_TYPE eStringType/* = RVST_SZ*/,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	if (NULL == hRootKey || NULL == pszSubKey || NULL == pszValue || NULL == pstrRet)
	{
		return -1;
	}

	std::wstring strRet;
	int nRet = GetRegStringW(hRootKey,
		(const WCHAR *)_bstr_t(pszSubKey),
		(const WCHAR *)_bstr_t(pszValue),
		&strRet,
		eStringType,
		eRaf);
	if (0 != nRet)
	{
		return nRet;
	}

	*pstrRet = (const char *)_bstr_t(strRet.c_str());

	return nRet;
}

int IUI::SetRegStringW(
	HKEY hRootKey,
	const WCHAR *pszSubKey,
	const WCHAR *pszValue,
	const WCHAR *pszData,
	REG_VALUE_STRING_TYPE eStringType/* = RVST_SZ*/,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	DWORD dwType = REG_SZ;
	switch(eStringType)
	{
	case RVST_SZ:
		dwType = REG_SZ;
		break;
	case RVST_EXPAND_SZ:
		dwType = REG_EXPAND_SZ;
		break;
	case RVST_MULTI_SZ:
		dwType = REG_MULTI_SZ;
		break;
	default:
		_ASSERT(FALSE);
		break;
	}

	return SetRegDataW(hRootKey, pszSubKey, pszValue, dwType, (const BYTE *)pszData,
		sizeof(WCHAR) * ((DWORD)wcslen(pszData) + 1), eRaf);
}

int IUI::GetRegStringW(
	HKEY hRootKey,
	const WCHAR *pszSubKey,
	const WCHAR *pszValue,
	std::wstring *pstrRet,
	REG_VALUE_STRING_TYPE eStringType/* = RVST_SZ*/,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	if (NULL == pstrRet)
	{
		return -1;
	}

	DWORD dwType = REG_SZ;
	switch (eStringType)
	{
	case RVST_SZ:
		dwType = REG_SZ;
		break;
	case RVST_EXPAND_SZ:
		dwType = REG_EXPAND_SZ;
		break;
	case RVST_MULTI_SZ:
		dwType = REG_MULTI_SZ;
		break;
	default:
		_ASSERT(FALSE);
		break;
	}

	std::vector<BYTE> vString;
	int nRet = GetRegDataW(hRootKey, pszSubKey, pszValue, dwType, &vString, eRaf);
	if (0 != nRet)
	{
		return nRet;
	}

	pstrRet->assign((WCHAR *)(&vString[0]), vString.size() / sizeof(WCHAR) - 1);
	return 0;
}

int IUI::DeleteRegValueW(HKEY hRootKey,
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;
	HKEY hKey = NULL;

	do
	{
		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY;
			break;
		default:
			break;
		}
		LSTATUS lRet = RegOpenKeyExW(hRootKey, pszSubKey, 0, samDesired, &hKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}

		// 注：RegSetValue仅能修改子键的默认“值”。修改非默认“值”，要使用RegSetValueEx
		lRet = RegDeleteValueW(hKey, pszValue);
		if (ERROR_SUCCESS != lRet)
		{
			break;
		}
	}
	while (false);

	if (NULL != hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

int IUI::DeleteRegKeyW(HKEY hRootKey, LPCWSTR pszSubKey, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;

	do 
	{
		// 递归删除键及子值和它们所有的值。
		LSTATUS lRet = SHDeleteKeyW(hRootKey, pszSubKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}
	} while (false);

	return nRet;
}

int IUI::QueryRegKeyW(HKEY hRootKey, LPCWSTR pszSubKey, BOOL *pbExist, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	if (NULL == pbExist)
	{
		return -1;
	}

	int nRet = 0;
	HKEY hKey = NULL;
	*pbExist = FALSE;

	do
	{
		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY;
			break;
		default:
			break;
		}

		LSTATUS lRet = ::RegOpenKeyExW(hRootKey, pszSubKey, 0, samDesired, &hKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}

		DWORD cSubKeys = 0;
		DWORD cbMaxSubKeyLen = 0;
		lRet = RegQueryInfoKeyW(hKey, NULL, NULL, NULL,
			&cSubKeys, &cbMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -3;
			break;
		}

		*pbExist = TRUE;
	} while (FALSE);

	if (NULL == hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return nRet;
}

int IUI::SetRegDword(HKEY hRootKey, LPCTSTR lpszSubKey, LPCTSTR lpszValue, DWORD dwData, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
#ifdef _UNICODE
	return SetRegDwordW(hRootKey, lpszSubKey, lpszValue, dwData, eRaf);
#else
	_ASSERT(FALSE);
	return -1;
#endif // _UNICODE
}

int IUI::GetRegDword(HKEY hRootKey, LPCTSTR lpszSubKey, LPCTSTR lpszValue, __out DWORD *pdwData, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
#ifdef _UNICODE
	return GetRegDwordW(hRootKey, lpszSubKey, lpszValue, pdwData, eRaf);
#else
	_ASSERT(FALSE);
	return -1;
#endif // _UNICODE
}

int IUI::EnumKeyW(HKEY hRootKey, LPCWSTR lpszSubKey, EnumKeyCallbackW fnEnumKeyCallback, void* pCallbackData, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;
	HKEY hKey = NULL;

	do
	{
		if (nullptr == fnEnumKeyCallback)
		{
			nRet = -1;
			break;
		}

		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY;
			break;
		default:
			break;
		}

		LSTATUS lRet = ::RegOpenKeyExW(hRootKey, lpszSubKey, 0, samDesired, &hKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}

		DWORD cbMaxSubKeyLen = 0;
		lRet = RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, &cbMaxSubKeyLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -3;
			break;
		}

		int nIndex = 0;
		int nRet = 0;
		CStringW strKey;
		WCHAR *pszBuffer = strKey.GetBufferSetLength(cbMaxSubKeyLen + 4);
		while (true)
		{
			lRet = RegEnumKeyW(hKey, nIndex, pszBuffer, cbMaxSubKeyLen + 4);
			if (ERROR_SUCCESS != lRet)
			{
				break;
			}

			nRet = fnEnumKeyCallback(hRootKey, lpszSubKey, pszBuffer, pCallbackData);
			if (0 != nRet)
			{
				break;
			}

			nIndex++;
		}
		strKey.ReleaseBuffer();

	} while (FALSE);

	if (NULL == hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return 0;
}

int IUI::EnumValueW(HKEY hRootKey, LPCWSTR lpszSubKey, EnumValueCallbackW fnEnumValueCallback, void* pCallbackData, REG_ACCESS_FLAG eRaf/* = RAF_DEFAULT*/)
{
	int nRet = 0;
	HKEY hKey = NULL;

	do
	{
		if (nullptr == fnEnumValueCallback)
		{
			nRet = -1;
			break;
		}

		REGSAM samDesired = KEY_ALL_ACCESS;
		switch (eRaf)
		{
		case IUI::RAF_DEFAULT:
			break;
		case IUI::RAF_32KEY:
			samDesired |= KEY_WOW64_32KEY; // 显式访问32位注册表视图
			break;
		case IUI::RAF_64KEY:
			samDesired |= KEY_WOW64_64KEY;
			break;
		default:
			break;
		}

		LSTATUS lRet = ::RegOpenKeyExW(hRootKey, lpszSubKey, 0, samDesired, &hKey);
		if (ERROR_SUCCESS != lRet)
		{
			nRet = -2;
			break;
		}

		int nIndex = 0;
		int nRet = 0;
		CStringW strKey;
		WCHAR* pszBuffer = strKey.GetBufferSetLength(16385);
		DWORD dwValueSize = 16385;
		DWORD dwValueType = REG_NONE;
		DWORD dwDataSize = 512;
		while (true)
		{
			// 得到data的长度
			pszBuffer[0] = 0;
			dwValueSize = 16385;
			lRet = RegEnumValueW(hKey, nIndex, pszBuffer, &dwValueSize, nullptr, &dwValueType, nullptr, &dwDataSize);
			if (ERROR_SUCCESS != lRet && ERROR_MORE_DATA != lRet)
			{
				break;
			}

			// 重新调用，得到value和data.
			pszBuffer[0] = 0;
			dwValueSize = 16385;
			std::vector<BYTE> vData;
			vData.resize(dwDataSize);
			lRet = RegEnumValueW(hKey, nIndex, pszBuffer, &dwValueSize, nullptr, &dwValueType, &vData[0], &dwDataSize);
			if (ERROR_SUCCESS != lRet)
			{
				break;
			}

			pszBuffer[dwValueSize] = 0;
			pszBuffer[dwValueSize + 1] = 0;

			nRet = fnEnumValueCallback(hRootKey, lpszSubKey, pszBuffer, dwValueType, &vData[0], dwDataSize, pCallbackData);
			if (0 != nRet)
			{
				break;
			}

			nIndex++;
		}
		strKey.ReleaseBuffer();

	} while (FALSE);

	if (NULL == hKey)
	{
		RegCloseKey(hKey);
		hKey = NULL;
	}

	return 0;
}

//int IUI::CatSubKeyW(CString * pstrSubKey, LPCWSTR lpszAppendSubKey)
//{
//	//if (nullptr == pstrSubKey || nullptr == lpszAppendSubKey)
//	//{
//	//	return -1;
//	//}
//
//	//if (pstrSubKey->Right(1) != L"\\"
//	//	&& pstrSubKey->Right(1) != L"/")
//	//{
//	//	(*pstrSubKey) += L"\\";
//	//}
//
//	//(*pstrSubKey) += lpszAppendSubKey;
//
//	return 0;
//}

int IUI::GoToRegSubKey(LPCWSTR lpszSubKey)
{
	ShellExecute(
		NULL,
		_T("open"),
		LR"(Regedit.exe)",
		NULL,
		NULL,
		SW_SHOWDEFAULT);

	HWND hReg = NULL;
	for (int i = 0; i < 10; ++i)
	{
		hReg = ::FindWindowEx(NULL, NULL, _T("RegEdit_RegEdit"), NULL);
		if (NULL != hReg)
		{
			break;
		}

		Sleep(500);
	}
	if (NULL == hReg)
	{
		_ASSERT(FALSE);
		return -2;
	}

	HWND hEdit = ::FindWindowEx(hReg, NULL, _T("Edit"), NULL);
	HWND hEdit2 = ::GetDlgItem(hReg, 0);
	if (hEdit != hEdit2)
	{
		_ASSERT(FALSE);
		return -2;
	}

	BOOL bRet = (BOOL)::SendMessageW(hEdit, WM_SETTEXT, 0,
		(LPARAM)lpszSubKey);

	::SendMessage(hEdit, WM_KEYDOWN, VK_RETURN, 0);

	return 0;
}
