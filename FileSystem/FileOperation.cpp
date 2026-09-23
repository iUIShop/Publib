#include "FileOperation.h"
#include <strsafe.h>

int IUI::CopyFolderW(LPCWSTR lpszSource, LPCWSTR lpszDest)
{
    // ±ØÐëË«Áã½áÎ²
    WCHAR szSrc[MAX_PATH] = { 0 };
    StringCchCopyW(szSrc, MAX_PATH, lpszSource);

    WCHAR szDst[MAX_PATH] = { 0 };
    StringCchCopyW(szDst, MAX_PATH, lpszDest);

    SHFILEOPSTRUCTW FileOp;
    SecureZeroMemory(&FileOp, sizeof(FileOp));

    FileOp.wFunc = FO_MOVE;
    FileOp.pFrom = szSrc;
    FileOp.pTo = szDst;
    FileOp.fFlags = FOF_NO_UI;

    int nOk = SHFileOperationW(&FileOp);
    int n = GetLastError();

    return nOk;
}
