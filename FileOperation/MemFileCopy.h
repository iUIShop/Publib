// 使用内存映射文件技术拷贝文件
// 支持取消拷贝、暂停拷贝\

// 测试：
// 拷贝过程中断线
// 取消拷贝
// 拷贝过程中平板关机，再开启
// 拷贝过程中平板熄屏，再开启
// 拷贝过程中删除源
// 拷贝过程中删除目标
// 多线程拷贝同一个文件。
// 返回错误代码。
// 测试大文件
// 测试各种类型文件。
// 测试空文件

#pragma once

#include <windows.h>

enum COPY_FILE_EVENT
{
    // CFFN: Copy File function.
    COPY_FILE_EVENT_START_COPY = 0,
    COPY_FILE_EVENT_ERR_ARG,
    COPY_FILE_EVENT_FINISH_WRITE,
    COPY_FILE_EVENT_FINISH_COPY,
    COPY_FILE_EVENT_CANCEL_OPERATIONS,
    COPY_FILE_EVENT_PAUSE_OPERATIONS,
    COPY_FILE_EVENT_CONTINUE_OPERATIONS,
    COPY_FILE_EVENT_UPDATE_PROGRESS,
    COPY_FILE_EVENT_WRITE_ERROR,

    COPY_FILE_EVENT_OPEN_SOURCE,                    // 打开源文件
    COPY_FILE_EVENT_GET_SOURCE_SIZE,                // 得到源文件大小
    COPY_FILE_EVENT_CREATE_SOURCE_FILE_MAPPING,     // 创建源文件内存映射
    COPY_FILE_EVENT_SOURCE_FILE_MAPPING,            // 源文件内存映射
    COPY_FILE_EVENT_CREATE_TARGET,                  // 创建目标文件
    COPY_FILE_EVENT_CREATE_TARGET_MAP,              // 创建目标文件内存映射
    COPY_FILE_EVENT_CREATE_TARGET_MAP_VIEW,         // 目标文件映射到内存
    COPY_FILE_EVENT_BLOCK_COUNT,                    // 块的数量，用于计算操作进度
};

typedef HRESULT(*COPYFILEPROGRESSCALLBACKPTR)(HANDLE hFile, COPY_FILE_EVENT eSinkType, WPARAM, LPARAM, void* pArg);

HANDLE FC_CopyFile(LPCWSTR lpszSrcFile, LPCWSTR lpszNewFileName, COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pfnArg);
int FC_CloseCopyFile(HANDLE h);
int FC_CancelCopyFile(HANDLE h);
HRESULT FC_GetCopyFileSource(HANDLE h, LPWSTR lpszBuffer, int cchBuff);
HRESULT FC_GetCopyFileDestFile(HANDLE h, LPWSTR lpszBuffer, int cchBuff);
int FC_GetTime(HANDLE h, __time64_t* pTime);
int FC_GetCopyTime(HANDLE h, __time64_t* pTime);
