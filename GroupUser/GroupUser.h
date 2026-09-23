#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace GroupUser
{
	struct CUser
	{
		std::wstring strLogonUser;
		std::wstring strDomainName;
	};

	// 功能：得到所有已登录的本地用户（不包含SYSTEM等用户，只包含在 "本地用户和组" 中列出的用户）
	// 原理：
	// 枚举所有会话，然后从会话得到用户名，没有登录的用户是没有会话的。所以，枚举到的会话，都是已登录的
	// 调用：
	// std::vector<GroupUser::CUser> vUsers;
	// GetAllLogonUsers(&vUsers, nullptr);
	int GetAllLogonUsers(std::vector<CUser>* pvUsers, DWORD* pdwError);

	// 功能：得到当前登录用户。支持从SYSTEM登录的服务进程得到当前登录的用户
	int GetCurLogonUser(std::wstring* pstrCurLogonUser, std::wstring* pstrDomainName, DWORD* pdwError);

	// 用法：
	//void main()
	//{
	//	std::wstring strUser;
	//	std::wstring strDomain;
	//	int nRet = GetCurLogonUser(&strUser, &strDomain, nullptr);

	//	// 如果strUser和strDomain为空，传给GetSIDFromUserName的第一个参数就是L"\\"，也会得到一个SID，但不是我们想要的。
	//	// 所以，这里要判断一下。
	//	if (0 != nRet
	//		|| strUser.empty()
	//		|| strDomain.empty())
	//	{
	//		return;
	//	}

	//	std::wstring strSid;
	//	strDomain += L"\\";
	//	strDomain += strUser;
	//	nRet = GetSIDFromUserName(strDomain.c_str(), &strSid, nullptr);
	//	if (0 != nRet)
	//	{
	//		return;
	//	}
	//}
	int GetSIDFromUserName(const wchar_t* pszUserName, std::wstring* pstrUserSID, DWORD* pdwError);

}
