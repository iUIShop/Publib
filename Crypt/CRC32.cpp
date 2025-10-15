#include "CRC32.h"
#include <atlstr.h>
#include <wincrypt.h>
#include <thread>

// 来自chatgpt，问：计算crc32最快的c++算法
// 动态生成一个表。下面通过查表来计算crc32。
void GenerateCRCTable(UINT32 crc32Table[256])
{
    const UINT32 polynomial = 0xEDB88320; // CRC32 多项式

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

// 支持大于4GB的文件和字节流
// 计算字节流的CRC32值
// bUpperCase：生成的CRC32是否大写，如果大写，为TRUE。
// strHashResult：保存得到的CRC32。
// 5.58GB的文件，计算耗时15秒（12th Gen Intel(R) Core(TM) i9-12900H   2.50 GHz， Windows 11 专业版22h2，固态硬盘）
// 作为对比，Notepad--耗时x多秒，MD5&SHA Checksum Utility 2.1耗时x秒。
int IUI::GetDataCRC32(const BYTE* pbtData, size_t nDataLen, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData)
{
    DWORD dw = GetTickCount();

    //
    // 开始
    //
    if (nullptr != OnCRC32Proc)
    {
        OnCRC32Proc(CM_BEGIN, 0, nDataLen, pUserData);
    }

    UINT32 crc32Table[256] = { 0 };
    GenerateCRCTable(crc32Table);


    //
    // 计算过程
    //
    UINT32 crc = 0xFFFFFFFF; // 初始 CRC32 值

    for (size_t i = 0; i < nDataLen; ++i)
    {
        BYTE index = (crc ^ pbtData[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32Table[index];

        if (nullptr != OnCRC32Proc)
        {
            // 为了提高性能，每1MB字节发送一次进度
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
    // 结束
    //
    DWORD nRet = crc ^ 0xFFFFFFFF; // 取反以得到最终的 CRC32 值
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

// 计算文件的CRC32值
// pszFilePath传入文件名
// bUpperCase：生成的CRC32是否大写，如果大写，为TRUE。
// strHashResult：保存得到的CRC32。
int IUI::GetFileCRC32(const std::wstring& strFilePath, BOOL bUpperCase, std::wstring *pstrHashResult, CRC32Proc OnCRC32Proc, void* pUserData)
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
 
            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"CreateFileW"), MAKELPARAM(2, dwError), pUserData);
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

            if (nullptr != OnCRC32Proc)
            {
                OnCRC32Proc(CM_ERROR, WPARAM(L"GetFileSizeEx"), MAKELPARAM(2, dwError), pUserData);
            }

            nRet = -3;
            break;
        }

        //
        // 创建文件映射
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
        // 映射文件到内存
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
        // 计算pFileData中数据的CRC32值。
        //
        DWORD dwRet = GetDataCRC32((BYTE*)pFileData, (SIZE_T)llFileSize.QuadPart, bUpperCase, pstrHashResult, OnCRC32Proc, pUserData);

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

int IUI::GetFileCRC32Async(const std::wstring& strFilePath, BOOL bUpperCase, CRC32Proc OnCRC32Proc, void* pUserData)
{
    std::thread t(GetFileCRC32, strFilePath, bUpperCase, nullptr, OnCRC32Proc, pUserData);
    t.detach();

    return 0;
}
