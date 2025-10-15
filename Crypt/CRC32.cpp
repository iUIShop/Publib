#include "CRC32.h"
#include <atlstr.h>
#include <wincrypt.h>
#include <thread>

// ����chatgpt���ʣ�����crc32����c++�㷨
// ��̬����һ��������ͨ�����������crc32��
void GenerateCRCTable(UINT32 crc32Table[256])
{
    const UINT32 polynomial = 0xEDB88320; // CRC32 ����ʽ

    for (UINT32 i = 0; i < 256; ++i)
    {
        UINT32 crc = i;
        for (int j = 0; j < 8; ++j)
        {
            crc = (crc >> 1) ^ ((crc & 1) * polynomial);
        }
        crc32Table[i] = crc;
    }
}

// ֧�ִ���4GB���ļ����ֽ���
// �����ֽ�����CRC32ֵ
// bUpperCase�����ɵ�CRC32�Ƿ��д�������д��ΪTRUE��
// strHashResult������õ���CRC32��
// 5.58GB���ļ��������ʱ15�루12th Gen Intel(R) Core(TM) i9-12900H   2.50 GHz�� Windows 11 רҵ��22h2����̬Ӳ�̣�
// ��Ϊ�Աȣ�Notepad--��ʱx���룬MD5&SHA Checksum Utility 2.1��ʱx�롣
int IUI::GetDataCRC32(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData)
{
    DWORD dw = GetTickCount();

    //
    // ��ʼ
    //
    if (nullptr != OnCRC32Proc)
    {
        OnCRC32Proc(CM_BEGIN, 0, nDataLen, pUserData);
    }

    UINT32 crc32Table[256] = { 0 };
    GenerateCRCTable(crc32Table);


    //
    // �������
    //
    UINT32 crc = 0xFFFFFFFF; // ��ʼ CRC32 ֵ

    for (size_t i = 0; i < nDataLen; ++i)
    {
        BYTE index = (crc ^ pbtData[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32Table[index];

        if (nullptr != OnCRC32Proc)
        {
            // Ϊ��������ܣ�ÿ1MB�ֽڷ���һ�ν���
            if (i % 1000000 == 0)
            {
                OnCRC32Proc(CM_PROGRESS, i + 1, nDataLen, pUserData);
            }
        }
    }
    if (nullptr != OnCRC32Proc)
    {
        OnCRC32Proc(CM_PROGRESS, nDataLen, nDataLen, pUserData);
    }

    //
    // ����
    //
    DWORD nRet = crc ^ 0xFFFFFFFF; // ȡ���Եõ����յ� CRC32 ֵ
    CStringW strCRC32;
    strCRC32.Format(bUpperCase ? L"%X" : L"%x", nRet);
    if (nullptr != pstrHashResult)
    {
        *pstrHashResult = (LPCWSTR)strCRC32;
    }

    if (nullptr != OnCRC32Proc)
    {
        OnCRC32Proc(CM_END, 0, WPARAM((LPCWSTR)strCRC32), pUserData);
    }

    DWORD d = GetTickCount() - dw;

    return 0;
}

// �����ļ���CRC32ֵ
// pszFilePath�����ļ���
// bUpperCase�����ɵ�CRC32�Ƿ��д�������д��ΪTRUE��
// strHashResult������õ���CRC32��
int IUI::GetFileCRC32(const std::wstring& strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData)
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
 
            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"CreateFileW"), MAKELPARAM(2, dwError), pUserData);
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

            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"GetFileSizeEx"), MAKELPARAM(2, dwError), pUserData);
            }

            nRet = -3;
            break;
        }

        //
        // �����ļ�ӳ��
        //
        hMappingHandle = CreateFileMappingW(hFile, NULL, PAGE_READONLY, llFileSize.HighPart, llFileSize.LowPart, NULL);
        if (nullptr == hMappingHandle)
        {
            DWORD dwError = GetLastError();

            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"CreateFileMappingW"), MAKELPARAM(2, dwError), pUserData);
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

            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"MapViewOfFile"), MAKELPARAM(2, dwError), pUserData);
            }

            nRet = -5;
            break;
        }

        //
        // ����pFileData�����ݵ�CRC32ֵ��
        //
        DWORD dwRet = GetDataCRC32((BYTE*)pFileData, (SIZE_T)llFileSize.QuadPart, bUpperCase, pstrHashResult, OnCRC32Proc, pUserData);

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

int IUI::GetFileCRC32Async(const std::wstring& strFilePath, BOOL bUpperCase, CRC32Proc OnCRC32Proc, void* pUserData)
{
    std::thread t(GetFileCRC32, strFilePath, bUpperCase, nullptr, OnCRC32Proc, pUserData);
    t.detach();

    return 0;
}
