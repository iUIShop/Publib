#include "MemFileCopy.h"

#include <crtdbg.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <strsafe.h>
#include <atomic>

// 经测试，比这个值小，拷贝慢，比它大，速度并未提升。
#define BUFSIZE 409600

// The handle of FC_CopyFile
struct FC_COPYFILE_HANDLE
{
    std::wstring m_strSrc;
    std::wstring m_strNewFileName;
    __time64_t m_timeCopyStart = 0;
    __time64_t m_timeCopyEnd = 0;
    __time64_t m_timeStart = 0;
    __time64_t m_timeEnd = 0;
    std::atomic<bool> m_bCanceled = false;
};


int WriteFile(HANDLE hFileCopy, LPCWSTR lpszNewFileName, const BYTE* pbtData, LARGE_INTEGER cbSize, COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pfnArg)
{
    int nRet = 0;
    HANDLE hFileTarget = INVALID_HANDLE_VALUE;
    HANDLE hMapTarget = nullptr;
    BYTE* pBufMapping = nullptr;

    do
    {
        if (nullptr == hFileCopy)
        {
            if (nullptr != fnProgress)
            {
                fnProgress(hFileCopy, COPY_FILE_EVENT_ERR_ARG, 0, 0, pfnArg);
            }

            nRet = -1;
            break;
        }

        FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
        pFileCopy->m_timeCopyStart = _time64(nullptr);

        //
        // 创建文件
        //
        hFileTarget = CreateFileW(
            lpszNewFileName,           // 文件名
            GENERIC_READ | GENERIC_WRITE,              // 写权限
            FILE_SHARE_READ,                         // 不共读
            NULL,                      // 默认安全属性
            CREATE_ALWAYS,             // 创建文件，如果文件存在则覆盖
            FILE_ATTRIBUTE_NORMAL,     // 正常文件属性
            NULL);                     // 不使用模板文件

        if (INVALID_HANDLE_VALUE == hFileTarget)
        {
            if (nullptr != fnProgress)
            {
                fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET, FALSE, 0, pfnArg);
            }

            nRet = -2;
            break;
        }
        if (nullptr != fnProgress)
        {
            fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET, TRUE, 0, pfnArg);
        }

        if (cbSize.QuadPart > 0 && nullptr != pbtData)
        {
            //
            // 创建内存映射文件, 注意：最后一个参数不能起名，如果起名，多线程压测的时候，只有一个线程
            // 创建的文件可以成功。但其它线程的不提示失败。
            //
            hMapTarget = CreateFileMappingW(
                hFileTarget,               // 文件句柄
                NULL,                      // 默认安全属性
                PAGE_READWRITE,            // 可读可写
                cbSize.HighPart,            // 最大文件大小的高32位
                cbSize.LowPart,             // 最大文件大小的低32位（1KB）
                nullptr);                   // 内存映射对象的名称
            if (nullptr == hMapTarget)
            {
                if (nullptr != fnProgress)
                {
                    fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET_MAP, FALSE, 0, pfnArg);
                }

                nRet = -3;
                break;
            }
            if (nullptr != fnProgress)
            {
                fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET_MAP, TRUE, 0, pfnArg);
            }

            //
            // 将文件内容映射到内存
            //
            pBufMapping = (BYTE*)MapViewOfFile(
                hMapTarget,                      // 内存映射对象句柄
                FILE_MAP_ALL_ACCESS,       // 可读可写
                0,                         // 文件偏移量的高32位
                0,                         // 文件偏移量的低32位
                cbSize.QuadPart);                     // 要映射的视图大小
            if (nullptr == pBufMapping)
            {
                if (nullptr != fnProgress)
                {
                    fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET_MAP_VIEW, FALSE, 0, pfnArg);
                }

                nRet = -4;
                break;
            }
            if (nullptr != fnProgress)
            {
                fnProgress(hFileCopy, COPY_FILE_EVENT_CREATE_TARGET_MAP_VIEW, TRUE, 0, pfnArg);
            }

            //
            // 计算分块
            //
            size_t nCount = cbSize.QuadPart / BUFSIZE;
            DWORD dwLastDataLen = cbSize.QuadPart % BUFSIZE;
            if (dwLastDataLen != 0)
            {
                nCount++;
            }
            if (nullptr != fnProgress)
            {
                fnProgress(hFileCopy, COPY_FILE_EVENT_BLOCK_COUNT, 0, nCount, pfnArg);
            }

            //
            // 分批次写入
            //
            DWORD dwLen = 0;    // 每次写入文件的数据长度
            size_t i = 0;
            int nCallbackRet = -1;
            BYTE* pWriteOffsetBuf = pBufMapping;
            for (i = 0; i < nCount; ++i)
            {
                if (pFileCopy->m_bCanceled)
                {
                    if (nullptr != fnProgress)
                    {
                        fnProgress(hFileCopy, COPY_FILE_EVENT_CANCEL_OPERATIONS, i, nCount, pfnArg);
                    }

                    break;
                }

                dwLen = BUFSIZE;

                // 最后一段数据
                if (i == nCount - 1 && dwLastDataLen != 0)
                {
                    dwLen = dwLastDataLen;
                }

                RtlCopyMemory(pWriteOffsetBuf, pbtData, dwLen);

                if (nullptr != fnProgress)
                {
                    int nRet = fnProgress(hFileCopy, COPY_FILE_EVENT_UPDATE_PROGRESS, i + 1, nCount, pfnArg);
                    if (0 != nRet)
                    {
                        // 取消拷贝
                        nRet = -5;
                        break;
                    }
                }

                pWriteOffsetBuf += dwLen;
                pbtData += dwLen;
            }
        }

        pFileCopy->m_timeCopyEnd = _time64(nullptr);

    } while (false);

    // 解除内存映射
    if (nullptr != pBufMapping)
    {
        UnmapViewOfFile(pBufMapping);
        pBufMapping = nullptr;
    }

    // 关闭内存映射文件句柄
    if (nullptr != hMapTarget)
    {
        CloseHandle(hMapTarget);
        hMapTarget = nullptr;
    }

    // 关闭文件句柄
    if (INVALID_HANDLE_VALUE != hFileTarget)
    {
        BOOL bRet = CloseHandle(hFileTarget);
        DWORD dwErr = GetLastError();
        hFileTarget = INVALID_HANDLE_VALUE;
    }

    if (nullptr != fnProgress)
    {
        fnProgress(hFileCopy, COPY_FILE_EVENT_FINISH_WRITE, 0, nRet, pfnArg);
    }

    return nRet;
}

HANDLE FC_CopyFile(LPCWSTR lpszSrcFile, LPCWSTR lpszNewFileName, COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pfnArg)
{
    int nRet = 0;

    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPVOID pFileData = nullptr;
    HANDLE hMappingHandle = nullptr;

    FC_COPYFILE_HANDLE* pFileCopy = new FC_COPYFILE_HANDLE;
    pFileCopy->m_strSrc = lpszSrcFile;
    pFileCopy->m_strNewFileName = lpszNewFileName;
    pFileCopy->m_timeStart = _time64(nullptr);

    do
    {
        if (nullptr != fnProgress)
        {
            fnProgress(pFileCopy, COPY_FILE_EVENT_START_COPY, 0, 0, pfnArg);
        }

        if (nullptr == lpszSrcFile
            || nullptr == lpszNewFileName)
        {
            if (nullptr != fnProgress)
            {
                fnProgress(pFileCopy, COPY_FILE_EVENT_ERR_ARG, 0, 0, pfnArg);
            }

            nRet = -1;
            break;
        }

        //
        // 打开文件
        //
        hFile = CreateFileW(lpszSrcFile,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwError = GetLastError();

            if (nullptr != fnProgress)
            {
                fnProgress(pFileCopy, COPY_FILE_EVENT_OPEN_SOURCE, FALSE, dwError, pfnArg);
            }

            nRet = -2;
            break;
        }
        _ASSERT(INVALID_HANDLE_VALUE != hFile);
        if (nullptr != fnProgress)
        {
            fnProgress(pFileCopy, COPY_FILE_EVENT_OPEN_SOURCE, TRUE, 0, pfnArg);
        }

        //
        // 获取文件大小
        //
        LARGE_INTEGER llFileSize;
        BOOL bRet = GetFileSizeEx(hFile, &llFileSize);
        if (!bRet)
        {
            DWORD dwError = GetLastError();

            if (nullptr != fnProgress)
            {
                fnProgress(pFileCopy, COPY_FILE_EVENT_GET_SOURCE_SIZE, FALSE, dwError, pfnArg);
            }

            nRet = -3;
            break;
        }
        if (nullptr != fnProgress)
        {
            fnProgress(pFileCopy, COPY_FILE_EVENT_GET_SOURCE_SIZE, TRUE, 0, pfnArg);
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

                if (nullptr != fnProgress)
                {
                    fnProgress(pFileCopy, COPY_FILE_EVENT_CREATE_SOURCE_FILE_MAPPING, FALSE, dwError, pfnArg);
                }

                nRet = -4;
                break;
            }
            if (nullptr != fnProgress)
            {
                fnProgress(pFileCopy, COPY_FILE_EVENT_CREATE_SOURCE_FILE_MAPPING, TRUE, 0, pfnArg);
            }

            //
            // 映射文件到内存
            //
            pFileData = MapViewOfFile(hMappingHandle, FILE_MAP_READ, 0, 0, (SIZE_T)llFileSize.QuadPart);
            if (nullptr == pFileData)
            {
                DWORD dwError = GetLastError();

                if (nullptr != fnProgress)
                {
                    fnProgress(pFileCopy, COPY_FILE_EVENT_SOURCE_FILE_MAPPING, FALSE, dwError, pfnArg);
                }

                nRet = -5;
                break;
            }
            if (nullptr != fnProgress)
            {
                fnProgress(pFileCopy, COPY_FILE_EVENT_SOURCE_FILE_MAPPING, TRUE, 0, pfnArg);
            }
        }

        //
        // 拷贝
        //
        int nValue = WriteFile(pFileCopy, lpszNewFileName, (BYTE*)pFileData, llFileSize, fnProgress, pfnArg);

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

    pFileCopy->m_timeEnd = _time64(nullptr);

    if (nullptr != fnProgress)
    {
        fnProgress(pFileCopy, COPY_FILE_EVENT_FINISH_COPY, 0, nRet, pfnArg);
    }

    return pFileCopy;
}

int FC_CloseCopyFile(HANDLE hFileCopy)
{
    if (nullptr == hFileCopy)
    {
        return 1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    delete pFileCopy;
    pFileCopy = nullptr;

    return 0;
}

int FC_CancelCopyFile(HANDLE hFileCopy)
{
    if (nullptr == hFileCopy)
    {
        return -1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    pFileCopy->m_bCanceled = true;

    return 0;
}

HRESULT FC_GetCopyFileSource(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff)
{
    if (nullptr == hFileCopy)
    {
        return -1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    return StringCchCopy(lpszBuffer, cchBuff, pFileCopy->m_strSrc.c_str());
}

HRESULT FC_GetCopyFileDestFile(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff)
{
    if (nullptr == hFileCopy)
    {
        return -1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    return StringCchCopy(lpszBuffer, cchBuff, pFileCopy->m_strNewFileName.c_str());
}

int FC_GetTime(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return -1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;

    *pTime = pFileCopy->m_timeEnd - pFileCopy->m_timeStart;

    return 0;
}

int FC_GetCopyTime(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return -1;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;

    *pTime = pFileCopy->m_timeCopyEnd - pFileCopy->m_timeCopyStart;

    return 0;
}
