#include "GroupUser.h"
#include <wtsapi32.h>
#include <memory>
#include <sddl.h>
#pragma comment(lib, "wtsapi32.lib")

void AutoWTSFreeMemory(WCHAR** p)
{
	if (nullptr != p && nullptr != *p)
	{
		WTSFreeMemory(*p);
		*p = nullptr;
	}
}

int GroupUser::GetAllLogonUsers(std::vector<CUser>* pvUsers, DWORD* pdwError)
{
	int nRet = 0;

	WTS_SESSION_INFO* pSessionInfo = nullptr;
	DWORD dwSessionCount = 0;

	do
	{
		if (nullptr == pvUsers)
		{
			nRet = -1;
			break;
		}

		if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwSessionCount))
		{
			if (nullptr != pdwError)
			{
				*pdwError = GetLastError();
			}

			nRet = -2;
			break;
		}

		for (DWORD i = 0; i < dwSessionCount; ++i)
		{
			WCHAR* pszUserName = nullptr;
			std::unique_ptr<WCHAR*, decltype(AutoWTSFreeMemory)*> upUserName(&pszUserName, AutoWTSFreeMemory);
			WCHAR* pszDomainName = nullptr;
			std::unique_ptr<WCHAR*, decltype(AutoWTSFreeMemory)*> upDomainName(&pszDomainName, AutoWTSFreeMemory);

			DWORD dwSessionId = pSessionInfo[i].SessionId;
			CUser user;

			DWORD cbUserNameSize = 0;
			if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSUserName, &pszUserName, &cbUserNameSize))
			{
				if (nullptr != pdwError)
				{
					*pdwError = GetLastError();
				}

				nRet = -3;
				break;
			}
			user.strLogonUser = pszUserName;

			DWORD cbDomainNameSize = 0;
			if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSDomainName, &pszDomainName, &cbDomainNameSize))
			{
				if (nullptr != pdwError)
				{
					*pdwError = GetLastError();
				}

				nRet = -4;
				break;
			}
			user.strDomainName = pszDomainName;

			if (!user.strLogonUser.empty())
			{
				pvUsers->push_back(user);
			}
		}

		if (0 != nRet)
		{
			break;
		}

	} while (false);

	if (nullptr != pSessionInfo)
	{
		WTSFreeMemory(pSessionInfo);
		pSessionInfo = nullptr;
	}

	return nRet;
}

int GroupUser::GetCurLogonUser(std::wstring* pstrCurLogonUser, std::wstring* pstrDomainName, DWORD* pdwError)
{
    int nRet = 0;

    WTS_SESSION_INFO* pSessionInfo = nullptr;
    WCHAR* pszUserName = nullptr;
    WCHAR* pszDomainName = nullptr;
    DWORD dwSessionCount = 0;

    do
    {
        if (nullptr == pstrCurLogonUser)
        {
            nRet = -1;
            break;
        }

        if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwSessionCount))
        {
            if (nullptr != pdwError)
            {
                *pdwError = GetLastError();
            }

            nRet = -2;
            break;
        }

        for (DWORD i = 0; i < dwSessionCount; ++i)
        {
            if (pSessionInfo[i].State == WTSActive)	// 测试一下，在通过远程桌面连接、且登录了多个用户的时候，与WTSGetActiveConsoleSessionId拿到的Session ID一样吗？
            {
                DWORD dwSessionId = pSessionInfo[i].SessionId;

                DWORD cbUserNameSize = 0;
                if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSUserName, &pszUserName, &cbUserNameSize))
                {
                    if (nullptr != pdwError)
                    {
                        *pdwError = GetLastError();
                    }

                    nRet = -3;
                    break;
                }

                *pstrCurLogonUser = pszUserName;

                if (nullptr != pstrDomainName)
                {
                    DWORD cbDomainNameSize = 0;
                    if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSDomainName, &pszDomainName, &cbDomainNameSize))
                    {
                        if (nullptr != pdwError)
                        {
                            *pdwError = GetLastError();
                        }

                        nRet = -4;
                        break;
                    }

                    *pstrDomainName = pszDomainName;
                }

                break;
            }
        }

        if (0 != nRet)
        {
            break;
        }

    } while (false);

    if (nullptr != pszDomainName)
    {
        WTSFreeMemory(pszDomainName);
        pszDomainName = nullptr;
    }

    if (nullptr != pszUserName)
    {
        WTSFreeMemory(pszUserName);
        pszUserName = nullptr;
    }

    if (nullptr != pSessionInfo)
    {
        WTSFreeMemory(pSessionInfo);
        pSessionInfo = nullptr;
    }

    return nRet;
}

// 使用完全限定的帐户名（例如，domain_name\user_name）而不是独立名称（例如，user_name）。
// 虽然GetSIDFromUserName也支持通过user_name查找，但完全限定的名称是明确的，在执行查找时能提供更好的性能。
int GroupUser::GetSIDFromUserName(const wchar_t* pszUserName, std::wstring* pstrUserSID, DWORD* pdwError)
{
    SID_NAME_USE eSidType = SidTypeUser;
    DWORD cbSidBufferSize = 0;
    DWORD cchDomainNameBufferLen = 0;
    SID* pSid = nullptr;
    wchar_t* pszDomainName = nullptr;
    wchar_t* pszSid = nullptr;
    int nRet = 0;

    do
    {
        if (nullptr == pstrUserSID)
        {
            nRet = -1;
            break;
        }

	// 判断pszUserName中是否包含\，如果不包含，说明不是domain_name\user_name格式，则返回错误。
	std::wstring strUser = pszUserName;
        std::wstring::size_type nPos = strUser.find('\\');
        if (nPos == std::wstring::npos)
        {
            // Not found domain
            nRet = -2;
            break;
        }


        // 获取SID和域名所需的缓冲区大小
        // LookupAccountNameW如果从本地查询不到，还会从域中查找，可能速度较慢
        BOOL bRet = LookupAccountNameW(nullptr, pszUserName, nullptr, &cbSidBufferSize, nullptr, &cchDomainNameBufferLen, &eSidType);
        if (0 == cbSidBufferSize)
        {
            nRet = -3;
            break;
        }

        pSid = (SID*)new BYTE[cbSidBufferSize];
        pszDomainName = new wchar_t[cchDomainNameBufferLen];

        // 获取SID和域名
        if (!LookupAccountNameW(nullptr, pszUserName, pSid, &cbSidBufferSize, pszDomainName, &cchDomainNameBufferLen, &eSidType))
        {
            if (nullptr != pdwError)
            {
                *pdwError = GetLastError();
            }

            nRet = -4;
            break;
        }

        // 将SID转换为字符串
        if (!ConvertSidToStringSidW(pSid, &pszSid))
        {
            if (nullptr != pdwError)
            {
                *pdwError = GetLastError();
            }

            nRet = -5;
            break;
        }

        *pstrUserSID = pszSid;

    } while (false);

    if (nullptr != pszSid)
    {
        LocalFree(pszSid);
        pszSid = nullptr;
    }

    if (nullptr != pszDomainName)
    {
        delete[] pszDomainName;
        pszDomainName = nullptr;
    }

    if (nullptr != pSid)
    {
        delete[](BYTE*)(pSid);
        pSid = nullptr;
    }

    return nRet;
}
