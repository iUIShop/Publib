#include "Privilege.h"


// 代码来自DeepSeek, 判断自己是不是被提权
BOOL IUI::IsElevated()
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD dwSize;

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        {
            fRet = elevation.TokenIsElevated;
        }

        CloseHandle(hToken);
    }

    return fRet;
}


// 代码来自DeepSeek，未验证
BOOL IUI::IsProcessElevatedAdmin(HANDLE hProcess)
{
    BOOL bIsElevated = FALSE;
    HANDLE hToken = NULL;

    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        return FALSE;

    // 检查令牌提升类型（适用于Windows Vista及以上）
    TOKEN_ELEVATION_TYPE elevType;
    DWORD dwSize;
    if (GetTokenInformation(hToken, TokenElevationType, &elevType, sizeof(elevType), &dwSize))
    {
        bIsElevated = (elevType == TokenElevationTypeFull);
    }
    else
    {
        // 处理不支持TokenElevationType的系统（如Windows XP）
        DWORD dwError = GetLastError();
        if (dwError == ERROR_INVALID_PARAMETER)
        {
            PSID pAdminSid = NULL;
            SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
            // 创建管理员组的SID
            if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &pAdminSid))
            {
                // 检查令牌是否属于管理员组
                CheckTokenMembership(hToken, pAdminSid, &bIsElevated);
                FreeSid(pAdminSid);
            }
        }
    }

    CloseHandle(hToken);
    return bIsElevated;
}

// 代码来自DeepSeek，判断第三方进程是否被提权
BOOL IUI::IsProcessElevatedAdmin(DWORD dwProcessId)
{
    //return ::IsUserAndAdmin();
    BOOL bIsElevated = FALSE;
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;

    do
    {
        // 打开目标进程（需要PROCESS_QUERY_LIMITED_INFORMATION权限）
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
        if (hProcess == NULL)
        {
            break;
        }

        // 获取进程令牌 (只能获取和本程序同级或低级的程序，否则会拒绝访问，所以，为了很广的适应性，本程序应该以管理员权限运行)
        if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken))
        {
            break;
        }

        // 方法1：检查令牌提升类型（Windows Vista+）
        TOKEN_ELEVATION_TYPE elevType;
        DWORD dwSize;
        if (GetTokenInformation(hToken, TokenElevationType, &elevType, sizeof(elevType), &dwSize))
        {
            bIsElevated = (elevType == TokenElevationTypeFull);
        }
        else
        {
            // 方法2：兼容旧系统，检查管理员组成员资格
            PSID pAdminSid = NULL;
            SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
            if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &pAdminSid))
            {
                if (!CheckTokenMembership(hToken, pAdminSid, &bIsElevated))
                {
                    //std::cerr << "CheckTokenMembership failed. Error: " << GetLastError() << std::endl;
                }
                FreeSid(pAdminSid);
            }
        }
    } while (FALSE);

    if (nullptr != hToken)
    {
        CloseHandle(hToken);
        hToken = nullptr;
    }
    if (nullptr != hProcess)
    {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }

    return bIsElevated;
}
