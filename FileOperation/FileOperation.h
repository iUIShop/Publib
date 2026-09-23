#pragma once

#include <windows.h>

// Sink type enumeration
enum SINK_TYPE_ENUM
{
    // CFFN: Copy File function.
    SINK_TYPE_CFFN_START_COPY = 0,
    SINK_TYPE_CFFN_ERR_ARG,                      // wParam: TRUE: suc.
    SINK_TYPE_CFFN_COM_INIT,                     // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_CREATE_FILEOPERATION,         // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_IFILEOPERATION,               // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_ADVISE,                       // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_OPEN_SOURCE,                  // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_CREATE_DEST_FOLDER,           // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_COPY_ITEM,                    // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_SET_OWNER_WINDOW,             // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_SET_OPERATION_FLAGS,          // wParam: TRUE: suc; FALSE: failed.
    SINK_TYPE_CFFN_PERFORMOPERATIONS,            // wParam: TRUE: suc; FALSE: failed. lParam: HRESULT
    SINK_TYPE_CFFN_FINISH_COPY,                  // After SINK_TYPE_FINISH_OPERATIONS, before terminate copy function. 

    // event in CFileOperationProgressSink
    SINK_TYPE_START_OPERATIONS,
    SINK_TYPE_FINISH_OPERATIONS,                // Finished
    SINK_TYPE_CANCEL_OPERATIONS,
    SINK_TYPE_PAUSE_OPERATIONS,
    SINK_TYPE_CONTINUE_OPERATIONS,
    SINK_TYPE_PRE_COPY_ITEM,
    SINK_TYPE_POST_COPY_ITEM,
    SINK_TYPE_UPDATE_PROGRESS
};

#define FC_ERR_INAVLID_PARAM                                -1
#define FC_ERR_COM_INIT                                     -2
#define FC_ERR_CREATE_FILEOPERATION                         -3
#define FC_ERR_ADVISE                                       -4
#define FC_ERR_OPEN_SOURCE                                  -5
#define FC_ERR_CREATE_TARGET_FOLDER                         -6
#define FC_ERR_COPY_ITEM                                    -7
#define FC_ERR_SET_OWNER_WINDOW                             -8
#define FC_ERR_SET_OPERATION_FLAGS                          -9
#define FC_ERR_PERFORM_OPERATIONS                           -10


typedef HRESULT (*COPYFILEPROGRESSCALLBACKPTR)(HANDLE hFile, SINK_TYPE_ENUM eSinkType, WPARAM, LPARAM, void *pArg);




// Need call FC_CloseCopyFile to close handle
HANDLE FC_CreateCopyFile(LPCWSTR lpszSrcFile, LPCWSTR lpszNewFileName);
int FC_SetCallback(HANDLE hFileCopy, COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pfnArg);
int FC_GetCallback(HANDLE hFileCopy, COPYFILEPROGRESSCALLBACKPTR* pfnProgress, void** pArg);
int FC_SetString(HANDLE hFileCopy, LPCWSTR lpszString);
HRESULT FC_GetString(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff);
int FC_SetLongPtr(HANDLE hFileCopy, LONG_PTR lUserData);
int FC_GetLongPtr(HANDLE hFileCopy, LONG_PTR* plUserData);
// Perform the operation
int FC_CopyFile(HANDLE pFile);
int FC_CloseCopyFile(HANDLE hFileCopy);
int FC_CancelCopyFile(HANDLE hFileCopy);
int FC_IsCancelCopyFile(HANDLE hFileCopy, BOOL* pbCancel);
HRESULT FC_GetCopyFileSource(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff);
HRESULT FC_GetCopyFileDestFile(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff);
int FC_GetCopyStart(HANDLE hFileCopy, __time64_t* pTime);
int FC_GetCopyEnd(HANDLE hFileCopy, __time64_t* pTime);
int FC_GetTotalStart(HANDLE hFileCopy, __time64_t* pTime);
int FC_GetTotalEnd(HANDLE hFileCopy, __time64_t* pTime);

// 可删除非空文件夹
int DeleteDirectoryW(LPCWSTR lpszDir);
