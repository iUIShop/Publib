#include "Hash256.h"
#include <atlstr.h>
#include <wincrypt.h>
#include <thread>

#define BUFSIZE 256 // �������ԣ�256�Ϳ��Դﵽ�������ܣ�������ֵ��С�����ܻἱ���½��������ó�1024��4096�������ó�256�����޲��
#define HASH256LEN 64

// ֧�ִ���4GB���ļ����ֽ���
// �����ֽ�����Hash256ֵ
// bUpperCase�����ɵ�Hash256�Ƿ��д�������д��ΪTRUE��
// strHashResult������õ���Hash256��
// 5.58GB���ļ��������ʱ5�루12th Gen Intel(R) Core(TM) i9-12900H   2.50 GHz�� Windows 11 רҵ��22h2����̬Ӳ�̣�
// ��Ϊ�Աȣ�Notepad--��ʱ40���룬MD5&SHA Checksum Utility 2.1��ʱ30�롣
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
        // ��ȡ�ض����ܷ����ṩ���� ��CSP�� ���ض���Կ�����ľ��
        // CryptAcquireContextW����API���Խ�����
        //
        bResult = CryptAcquireContextW(&hProv,
            NULL,                   // �� dwFlags ����Ϊ CRYPT_VERIFYCONTEXT ʱ��pszContainer ��������Ϊ NULL
            NULL,                   // ����˲���Ϊ NULL����ʹ���û�Ĭ���ṩ����
            PROV_RSA_AES,           // PROV_RSA_AES�ṩ��������ͬʱ֧������ǩ�������ݼ���
            CRYPT_VERIFYCONTEXT);   // ��ѡ��������ʹ����ʱ��Կ��Ӧ�ó��򣬻���Ҫ���ʳ־�˽Կ��Ӧ�ó��������ִ�й�ϣ�����ܺ�����ǩ����֤��Ӧ�ó���
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
        // �����������Ĺ�ϣ�������������ܷ����ṩ���� ��CSP�� ��ϣ����ľ�������䷵�ص�����Ӧ�ó���
        // �˾�����ڶ� CryptHashData �� CryptHashSessionKey �ĺ������ã��Թ�ϣ�Ự��Կ��������������
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
        // ������ӵ�ָ���Ĺ�ϣ���󡣿��Զ�ε��ô˺����� CryptHashSessionKey �����㳤�������������Ĺ�ϣ��
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
        // ���������
        //
        DWORD dwLen = 0;
        size_t i = 0;
        int nCallbackRet = -1;
        for (i = 0; i < nCount; ++i)
        {
            dwLen = BUFSIZE;

            // ���һ������
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
        // �������ƹ�ϣ������������ݡ�����ʹ�ô˺�������ʵ�ʹ�ϣֵ��
        // ���� CryptGetHashParam �󣬲��������ϣ������κ����ݡ�
        // �� CryptHashData �� CryptHashSessionKey ����������ʧ�ܡ�
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
        // ���ֽ�תΪ16�����ַ���
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
        // ��ɣ�֪ͨ������Ⱥ͵õ���Hash256ֵ
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

// �����ļ���Hash256ֵ
// pszFilePath�����ļ���
// bUpperCase�����ɵ�Hash256�Ƿ��д�������д��ΪTRUE��
// strHashResult������õ���Hash256��
int IUI::GetFileHash256(const std::wstring& strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, Hash256Proc OnHash256Proc, void* pUserData)
{
    int nRet = 0;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPVOID pFileData = nullptr;
    HANDLE hMappingHandle = nullptr;

    do
    {
        //
        // ���ļ�
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
        // ��ȡ�ļ���С
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
            // �����ļ�ӳ��
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
            // ӳ���ļ����ڴ�
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
        // ����pFileData�����ݵ�Hash256ֵ��
        //
        nRet = GetDataHash256((BYTE*)pFileData, (SIZE_T)llFileSize.QuadPart, bUpperCase, pstrHashResult, OnHash256Proc, pUserData);

    } while (false);

    // �ر��ڴ�ӳ���ļ�
    // �ر��ڴ�ӳ���ļ�
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
