#include "Hash256.h"
#include <atlstr.h>
#include <wincrypt.h>
#include <thread>

#define BUFSIZE 256 // 经过测试，256就可以达到最优性能，如果这个值减小，性能会急剧下将，但设置成1024或4096，与设置成256几乎无差别。
#define HASH256LEN 64

// 支持大于4GB的文件和字节流
// 计算字节流的Hash256值
// bUpperCase：生成的Hash256是否大写，如果大写，为TRUE。
// strHashResult：保存得到的Hash256。
// 5.58GB的文件，计算耗时5秒（12th Gen Intel(R) Core(TM) i9-12900H   2.50 GHz， Windows 11 专业版22h2，固态硬盘）
// 作为对比，Notepad--耗时40多秒，MD5&SHA Checksum Utility 2.1耗时30秒。
int IUI::GetDataHash256(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData)
{
    int nRet = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;

    BYTE btHash[HASH256LEN] = { 0 };
    DWORD cbHash = HASH256LEN;

    do
    {
        //
        // 获取特定加密服务提供程序 （CSP） 中特定密钥容器的句柄
        // CryptAcquireContextW这套API可以将废弃
        //
        bResult = CryptAcquireContextW(&hProv,
            NULL,                   // 当 dwFlags 设置为 CRYPT_VERIFYCONTEXT 时，pszContainer 必须设置为 NULL
            NULL,                   // 如果此参数为 NULL，则使用用户默认提供程序。
            PROV_RSA_AES,           // PROV_RSA_AES提供程序类型同时支持数字签名和数据加密
            CRYPT_VERIFYCONTEXT);   // 此选项适用于使用临时密钥的应用程序，或不需要访问持久私钥的应用程序，例如仅执行哈希、加密和数字签名验证的应用程序。
        if (!bResult)
        {
            DWORD dwError = GetLastError();

            if (nullptr != OnHash256Proc)
            {
                OnHash256Proc(HM_ERROR, WPARAM(L"CryptAcquireContextW"), MAKELPARAM(3, dwError), pUserData);
            }

            nRet = -2;
            break;
        }
        _ASSERT(NULL != hProv);

        //
        // 启动数据流的哈希处理。它创建加密服务提供程序 （CSP） 哈希对象的句柄并将其返回到调用应用程序。
        // 此句柄用于对 CryptHashData 和 CryptHashSessionKey 的后续调用，以哈希会话密钥和其他数据流。
        //
        bResult = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
        if (!bResult)
        {
            DWORD dwError = GetLastError();

            if (nullptr != OnHash256Proc)
            {
                OnHash256Proc(HM_ERROR, WPARAM(L"CryptCreateHash"), MAKELPARAM(3, dwError), pUserData);
            }

            nRet = -3;
            break;
        }
        _ASSERT(NULL != hHash);

        //
        // 数据添加到指定的哈希对象。可以多次调用此函数和 CryptHashSessionKey 来计算长或不连续数据流的哈希。
        //
        size_t nCount = nDataLen / BUFSIZE;
        DWORD dwLastDataLen = nDataLen % BUFSIZE;
        if (dwLastDataLen != 0)
        {
            nCount++;
        }

        if (nullptr != OnHash256Proc)
        {
            OnHash256Proc(HM_BEGIN, 0, nCount, pUserData);
        }

        //
        // 分批次添加
        //
        DWORD dwLen = 0;
        size_t i = 0;
        int nCallbackRet = -1;
        for (i = 0; i < nCount; ++i)
        {
            dwLen = BUFSIZE;

            // 最后一段数据
            if (i == nCount - 1 && dwLastDataLen != 0)
            {
                dwLen = dwLastDataLen;
            }

            bResult = CryptHashData(hHash, pbtData, dwLen, 0);
            if (!bResult)
            {
                DWORD dwError = GetLastError();
                nRet = -4;
                break;
            }

            if (nullptr != OnHash256Proc)
            {
                nCallbackRet = OnHash256Proc(HM_PROGRESS, i + 1, nCount, pUserData);
                if (0 != nCallbackRet)
                {
                    nRet = -7;
                    break;
                }
            }

            pbtData += dwLen;
        }

        if (0 != nRet)
        {
            nRet = -5;
            break;
        }

        //
        // 检索控制哈希对象操作的数据。可以使用此函数检索实际哈希值。
        // 调用 CryptGetHashParam 后，不能再向哈希中添加任何数据。
        // 对 CryptHashData 或 CryptHashSessionKey 的其他调用失败。
        //
        bResult = CryptGetHashParam(hHash, HP_HASHVAL, btHash, &cbHash, 0);
        if (!bResult)
        {
            DWORD dwError = GetLastError();

            if (nullptr != OnHash256Proc)
            {
                OnHash256Proc(HM_ERROR, WPARAM(L"CryptGetHashParam"), MAKELPARAM(3, dwError), pUserData);
            }

            nRet = -6;
            break;
        }

        //
        // 把字节转为16进制字符串
        //
        CStringW strHash256;
        for (DWORD j = 0; j < cbHash; ++j)
        {
            int nHigh = btHash[j] >> 4;
            int nLow = btHash[j] & 0xf;

            CStringW str;
            str.Format(bUpperCase ? L"%X%X" : L"%x%x", nHigh, nLow);

            strHash256 += str;
        }

        //
        // 完成，通知外面进度和得到的Hash256值
        //
        if (nullptr != OnHash256Proc)
        {
            OnHash256Proc(HM_END, 0, (LPARAM)(LPCWSTR)strHash256, pUserData);
        }

        if (nullptr != pstrHashResult)
        {
            *pstrHashResult = (LPCWSTR)strHash256;
        }

    } while (false);

    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
        hHash = NULL;
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
        hProv = NULL;
    }

    return nRet;
}

// 计算文件的Hash256值
// pszFilePath传入文件名
// bUpperCase：生成的Hash256是否大写，如果大写，为TRUE。
// strHashResult：保存得到的Hash256。
int IUI::GetFileHash256(const std::wstring& strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData)
{
    int nRet = 0;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPVOID pFileData = nullptr;
    HANDLE hMappingHandle = nullptr;

    do
    {
        //
        // 打开文件
        //
        hFile = CreateFileW(strFilePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwError = GetLastError();
 
            if (nullptr != OnHash256Proc)
            {
                OnHash256Proc(HM_ERROR, WPARAM(L"CreateFileW"), MAKELPARAM(2, dwError), pUserData);
            }

            nRet = -2;
            break;
        }
        _ASSERT(INVALID_HANDLE_VALUE != hFile);

        //
        // 获取文件大小
        //
        LARGE_INTEGER llFileSize;
        BOOL bRet = GetFileSizeEx(hFile, &llFileSize);
        if (!bRet)
        {
            DWORD dwError = GetLastError();

            if (nullptr != OnHash256Proc)
            {
                OnHash256Proc(HM_ERROR, WPARAM(L"GetFileSizeEx"), MAKELPARAM(2, dwError), pUserData);
            }

            nRet = -3;
            break;
        }

        if (0 == llFileSize.QuadPart)
        {
            pFileData = nullptr;
        }
        else
        {
            //
            // 创建文件映射
            //
            hMappingHandle = CreateFileMappingW(hFile, NULL, PAGE_READONLY, llFileSize.HighPart, llFileSize.LowPart, NULL);
            if (nullptr == hMappingHandle)
            {
                DWORD dwError = GetLastError();

                if (nullptr != OnHash256Proc)
                {
                    OnHash256Proc(HM_ERROR, WPARAM(L"CreateFileMappingW"), MAKELPARAM(2, dwError), pUserData);
                }

                nRet = -4;
                break;
            }

            //
            // 映射文件到内存
            //
            pFileData = MapViewOfFile(hMappingHandle, FILE_MAP_READ, 0, 0, (SIZE_T)llFileSize.QuadPart);
            if (nullptr == pFileData)
            {
                DWORD dwError = GetLastError();

                if (nullptr != OnHash256Proc)
                {
                    OnHash256Proc(HM_ERROR, WPARAM(L"MapViewOfFile"), MAKELPARAM(2, dwError), pUserData);
                }

                nRet = -5;
                break;
            }
        }

        //
        // 计算pFileData中数据的Hash256值。
        //
        nRet = GetDataHash256((BYTE*)pFileData, (SIZE_T)llFileSize.QuadPart, bUpperCase, pstrHashResult, OnHash256Proc, pUserData);

    } while (false);

    // 关闭内存映射文件
    // 关闭内存映射文件
    if (nullptr != pFileData)
    {
        UnmapViewOfFile(pFileData);
        pFileData = nullptr;
    }

    if (nullptr != hMappingHandle)
    {
        CloseHandle(hMappingHandle);
        hMappingHandle = nullptr;
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return nRet;
}

int IUI::GetFileHash256Async(const std::wstring& strFilePath, BOOL bUpperCase, Hash256Proc OnHash256Proc, void* pUserData)
{
    std::thread t(GetFileHash256, strFilePath, bUpperCase, nullptr, OnHash256Proc, pUserData);
    t.detach();

    return 0;
}
