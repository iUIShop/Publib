#include <windows.h>
#include <Shobjidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <thread>
#include <mutex>
#include "FileOperation.h"

#pragma comment (lib, "Shlwapi.lib")

// Max buffer size for displaying sink messages in list view
#define MAX_BUFF 1024


class CFileOperationProgressSink : public IFileOperationProgressSink
{
public:
    CFileOperationProgressSink() : _cRef(1)
    {
    }
    ~CFileOperationProgressSink() {}

public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CFileOperationProgressSink, IFileOperationProgressSink),
            {0},
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (0 == cRef)
        {
            delete this;
        }
        return cRef;
    }

    // IFileOperationProgressSink
    IFACEMETHODIMP StartOperations();
    IFACEMETHODIMP FinishOperations(HRESULT hrResult);
    IFACEMETHODIMP PreRenameItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/, PCWSTR /*pszNewName*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PostRenameItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/, PCWSTR /*pszNewName*/, HRESULT /*hrRename*/, IShellItem* /*psiNewlyCreated*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PreMoveItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/, IShellItem* /*psiDestinationFolder*/, PCWSTR /*pszNewName*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PostMoveItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/,
        IShellItem* /*psiDestinationFolder*/, PCWSTR /*pszNewName*/, HRESULT /*hrNewName*/, IShellItem* /*psiNewlyCreated*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PreCopyItem(DWORD dwFlags, IShellItem* psiItem,
        IShellItem* psiDestinationFolder, PCWSTR pszNewName);
    IFACEMETHODIMP PostCopyItem(DWORD dwFlags, IShellItem* psiItem,
        IShellItem* psiDestinationFolder, PCWSTR pwszNewName, HRESULT hrCopy,
        IShellItem* psiNewlyCreated);
    IFACEMETHODIMP PreDeleteItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PostDeleteItem(DWORD /*dwFlags*/, IShellItem* /*psiItem*/, HRESULT /*hrDelete*/, IShellItem* /*psiNewlyCreated*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PreNewItem(DWORD /*dwFlags*/, IShellItem* /*psiDestinationFolder*/, PCWSTR /*pszNewName*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP PostNewItem(DWORD /*dwFlags*/, IShellItem* /*psiDestinationFolder*/,
        PCWSTR /*pszNewName*/, PCWSTR /*pszTemplateName*/, DWORD /*dwFileAttributes*/, HRESULT /*hrNew*/, IShellItem* /*psiNewItem*/)
    {
        return S_OK;
    }
    IFACEMETHODIMP UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar);
    IFACEMETHODIMP ResetTimer()
    {
        return S_OK;
    }
    IFACEMETHODIMP PauseTimer()
    {
        return S_OK;
    }
    IFACEMETHODIMP ResumeTimer()
    {
        return S_OK;
    }

public:
    void SetProgressCallback(COPYFILEPROGRESSCALLBACKPTR fnProgress, void *pArg);
    int Cancel();
    bool IsCancel() const;
    int Pause();
    int Continue();
    int SetCopyFileHandle(HANDLE hCopyFile)
    {
        m_hCopyFileHandle = hCopyFile;
        return 0;
    }

private:
    HRESULT OnUpdateProgress(SINK_TYPE_ENUM eSinkType, WPARAM wParam, LPARAM lParam);

    long _cRef = 0;
    HANDLE m_hCopyFileHandle = nullptr;
    COPYFILEPROGRESSCALLBACKPTR m_fnProgress = nullptr;
    void* m_pProgressFnArg = nullptr;
    std::atomic<bool> m_bCanceled = false;
    std::atomic<bool> m_bPaused = false;
};


// The handle of FC_CopyFile
struct FC_COPYFILE_HANDLE
{
    CFileOperationProgressSink* m_pSink;
    std::wstring m_strSrc;
    std::wstring m_strDestFolder;
    std::wstring m_strFullNewFileName;
    std::wstring m_strOnlyNewFileName;
    std::wstring m_strUserData;
    LONG_PTR m_lUserData;

    HWND m_hWndNotify = nullptr;
    COPYFILEPROGRESSCALLBACKPTR m_fnProgressCallback = nullptr;
    void* m_pProgressCallbackArg = nullptr;

    std::atomic<__time64_t> m_timeCopyStart = 0;
    std::atomic<__time64_t> m_timeCopyEnd = 0;
    std::atomic<__time64_t> m_timeTotalStart = 0;
    std::atomic<__time64_t> m_timeTotalEnd = 0;
};


// IFileOperationProgressSink
IFACEMETHODIMP CFileOperationProgressSink::StartOperations()
{
    OnUpdateProgress(SINK_TYPE_START_OPERATIONS, NULL, 0);
    return S_OK;
}

// IFileOperationProgressSink
// Trigger in PerformOperations, but the file closed.
IFACEMETHODIMP CFileOperationProgressSink::FinishOperations(HRESULT)
{
    OnUpdateProgress(SINK_TYPE_FINISH_OPERATIONS, NULL, 0);
    return S_OK;
}

// IFileOperationProgressSink
IFACEMETHODIMP CFileOperationProgressSink::PreCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder, PCWSTR)
{
    PWSTR pszItem;
    HRESULT hr = psiItem->GetDisplayName(SIGDN_FILESYSPATH, &pszItem);
    if (SUCCEEDED(hr))
    {
        PWSTR pszDest;
        hr = psiDestinationFolder->GetDisplayName(SIGDN_FILESYSPATH, &pszDest);
        if (SUCCEEDED(hr))
        {
            WCHAR szBuff[MAX_BUFF] = { 0 };
            hr = StringCchPrintf(szBuff, ARRAYSIZE(szBuff), L"Flags: %u, Item: %s, Destination: %s",
                dwFlags, pszItem, pszDest);
            if (SUCCEEDED(hr))
            {
                OnUpdateProgress(SINK_TYPE_PRE_COPY_ITEM, (WPARAM)psiItem, (LPARAM)psiDestinationFolder);
            }
            CoTaskMemFree(pszDest);
        }
        CoTaskMemFree(pszItem);
    }
    return S_OK;
}

// IFileOperationProgressSink
IFACEMETHODIMP CFileOperationProgressSink::PostCopyItem(DWORD dwFlags, IShellItem* psiItem, IShellItem* psiDestinationFolder,
    PCWSTR, HRESULT hrCopy, IShellItem*)
{
    PWSTR pszItem;
    HRESULT hr = psiItem->GetDisplayName(SIGDN_FILESYSPATH, &pszItem);
    if (SUCCEEDED(hr))
    {
        PWSTR pszDest;
        hr = psiDestinationFolder->GetDisplayName(SIGDN_FILESYSPATH, &pszDest);
        if (SUCCEEDED(hr))
        {
            WCHAR szBuff[MAX_BUFF] = { 0 };
            hr = StringCchPrintf(szBuff, ARRAYSIZE(szBuff),
                L"Flags: %u, HRESULT: 0x%x, Item: %s, Destination: %s",
                dwFlags, hrCopy, pszItem, pszDest);
            if (SUCCEEDED(hr))
            {
                OnUpdateProgress(SINK_TYPE_POST_COPY_ITEM, (WPARAM)psiItem, LPARAM(psiDestinationFolder));
            }
            CoTaskMemFree(pszDest);
        }
        CoTaskMemFree(pszItem);
    }
    return S_OK;
}

// IFileOperationProgressSink
// UpdateProgress工作在哪个线程，取决于IFileOperationProgressSink对象在哪个线程中创建。
IFACEMETHODIMP CFileOperationProgressSink::UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar)
{
    if (m_bCanceled)
    {
        OnUpdateProgress(SINK_TYPE_CANCEL_OPERATIONS, iWorkTotal, iWorkSoFar);

        return E_FAIL;
    }

    while (m_bPaused)
    {
        Sleep(1);
    }

    HRESULT hr = OnUpdateProgress(SINK_TYPE_UPDATE_PROGRESS, iWorkTotal, iWorkSoFar);

    return hr;
}

void CFileOperationProgressSink::SetProgressCallback(COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pArg)
{
    m_fnProgress = fnProgress;
    m_pProgressFnArg = pArg;
}

int CFileOperationProgressSink::Cancel()
{
    m_bCanceled = true;
    return 0;
}

bool CFileOperationProgressSink::IsCancel() const
{
    return m_bCanceled;
}

int CFileOperationProgressSink::Pause()
{
    m_bPaused = true;
    return 0;
}

int CFileOperationProgressSink::Continue()
{
    m_bCanceled = false;
    m_bPaused = false;

    return 0;
}

HRESULT CFileOperationProgressSink::OnUpdateProgress(SINK_TYPE_ENUM eSinkType, WPARAM wParam, LPARAM lParam)
{
    // Notify caller.
    if (nullptr != m_fnProgress)
    {
        return m_fnProgress(m_hCopyFileHandle, eSinkType, wParam, lParam, m_pProgressFnArg);
    }

    return S_OK;
}

// Need call FC_CloseCopyFile to close handle
HANDLE FC_CreateCopyFile(LPCWSTR lpszSrcFile, LPCWSTR lpszNewFileName)
{
    FC_COPYFILE_HANDLE* pFile = new FC_COPYFILE_HANDLE;
    if (nullptr == pFile)
    {
        return nullptr;
    }

    pFile->m_strSrc = lpszSrcFile;
    pFile->m_strFullNewFileName = lpszNewFileName;
    pFile->m_strOnlyNewFileName = PathFindFileName(lpszNewFileName);

    WCHAR szFolder[MAX_PATH] = { 0 };
    StringCchCopy(szFolder, MAX_PATH, lpszNewFileName);
    PathRemoveFileSpec(szFolder);
    
    pFile->m_strDestFolder = szFolder;

    return pFile;
}

int FC_SetCallback(HANDLE hFileCopy, COPYFILEPROGRESSCALLBACKPTR fnProgress, void* pfnArg)
{
    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    if (nullptr == pFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    pFileCopy->m_fnProgressCallback = fnProgress;
    pFileCopy->m_pProgressCallbackArg = pfnArg;

    return 0;
}

int FC_GetCallback(HANDLE hFileCopy, COPYFILEPROGRESSCALLBACKPTR* pfnProgress, void** pArg)
{
    if (nullptr == hFileCopy || nullptr == pfnProgress || nullptr == pArg)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    *pfnProgress = pFileCopy->m_fnProgressCallback;
    *pArg = pFileCopy->m_pProgressCallbackArg;

    return 0;
}

int FC_SetString(HANDLE hFileCopy, LPCWSTR lpszString)
{
    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    if (nullptr == pFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    pFileCopy->m_strUserData = lpszString;

    return 0;
}

HRESULT FC_GetString(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    return StringCchCopy(lpszBuffer, cchBuff, pFileCopy->m_strUserData.c_str());
}

int FC_SetLongPtr(HANDLE hFileCopy, LONG_PTR lUserData)
{
    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    if (nullptr == pFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    pFileCopy->m_lUserData = lUserData;

    return 0;
}

int FC_GetLongPtr(HANDLE hFileCopy, LONG_PTR *plUserData)
{
    if (nullptr == hFileCopy || nullptr == plUserData)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    *plUserData = pFileCopy->m_lUserData;

    return 0;
}

// Perform the operation
int FC_CopyFile(HANDLE pFile)
{
    int nRet = 0;
    HRESULT hr = S_OK;
    IFileOperation* pfo = nullptr;
    DWORD dwCookie = 0;
    BOOL bAdvise = FALSE;
    IShellItem* psiFrom = nullptr;
    IShellItem* psiTo = nullptr;

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)pFile;

    do
    {
        if (nullptr == pFileCopy)
        {
            nRet = FC_ERR_INAVLID_PARAM;
            break;
        }

        pFileCopy->m_timeTotalStart = _time64(nullptr);

        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_START_COPY, 0, 0, pFileCopy->m_pProgressCallbackArg);
        }

        if (pFileCopy->m_strSrc.empty()
            || pFileCopy->m_strDestFolder.empty())
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_ERR_ARG, 0, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_INAVLID_PARAM;
            break;
        }


        //
        // COM init
        // 在下载nfs文件的时候：
        // 如果初始化为多线程，虽然多个线程同时下载同一个文件，也是一个成功后再下载另一个，
        // 这个可以保证下载后的文件不被自动取消（即下载后不被删除）
        // 如果初始化为单线程，虽然多个线程同时下载同一个文件，但下载早的，到99%的时候，就停止不动了，
        // 最终大概率这个文件被自动取消，目标文件被自动删除。
        // 在拷贝本地文件的时候，初始化为单线程也没有任何问题。但初始化为多线程，VS和程序都会Hang住。
        //
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (!SUCCEEDED(hr))
        {
            if (RPC_E_CHANGED_MODE != hr && S_FALSE != hr)
            {
                if (nullptr != pFileCopy->m_fnProgressCallback)
                {
                    pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_COM_INIT, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
                }

                nRet = FC_ERR_COM_INIT;
                break;
            }
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_COM_INIT, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Create the file operation object
        //
        hr = CoCreateInstance(__uuidof(FileOperation), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_CREATE_FILEOPERATION, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_CREATE_FILEOPERATION;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_CREATE_FILEOPERATION, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Setup our callback interface (IFileOperationProgressSink)
        //
        CFileOperationProgressSink fops;
        fops.SetProgressCallback(pFileCopy->m_fnProgressCallback, pFileCopy->m_pProgressCallbackArg);
        fops.SetCopyFileHandle(pFile);
        ((FC_COPYFILE_HANDLE*)pFile)->m_pSink = &fops;

        hr = pfo->Advise(&fops, &dwCookie);
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_ADVISE, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_ADVISE;
            break;
        }
        bAdvise = TRUE;
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_ADVISE, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Create an IShellItem from the supplied source path
        //
        hr = SHCreateItemFromParsingName(pFileCopy->m_strSrc.c_str(), NULL, IID_PPV_ARGS(&psiFrom));
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_OPEN_SOURCE, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_OPEN_SOURCE;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_OPEN_SOURCE, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Create an IShellItem from the supplied path
        // 
        hr = SHCreateItemFromParsingName(pFileCopy->m_strDestFolder.c_str(), NULL, IID_PPV_ARGS(&psiTo));
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_CREATE_DEST_FOLDER, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_CREATE_TARGET_FOLDER;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_CREATE_DEST_FOLDER, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Add the copy operation.  We do not add the IFileOperationProgressSink
        // here since we already did this in call to Advise().  If you add it
        // again here you will get duplicate sink notifications for the PreCopyItem
        // and PostCopyItem.
        // the 3th arg not include path, only new file name.
        //
        hr = pfo->CopyItem(psiFrom, psiTo, pFileCopy->m_strOnlyNewFileName.empty() ? nullptr : pFileCopy->m_strOnlyNewFileName.c_str(), nullptr);
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_COPY_ITEM, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_COPY_ITEM;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_COPY_ITEM, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Set the main dialog as the owner of any UI (progress, confirmations)
        // If no set, the copy UI as a top level dialog.
        //
        if (nullptr != pFileCopy->m_hWndNotify)
        {
            hr = pfo->SetOwnerWindow(pFileCopy->m_hWndNotify);
            if (!SUCCEEDED(hr))
            {
                if (nullptr != pFileCopy->m_fnProgressCallback)
                {
                    pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_SET_OWNER_WINDOW, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
                }

                nRet = FC_ERR_SET_OWNER_WINDOW;
                break;
            }
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_SET_OWNER_WINDOW, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
            }
        }

        //
        // Set our default operation flags for the operation
        //
        hr = pfo->SetOperationFlags(FOF_NOCONFIRMMKDIR | FOFX_NOCOPYHOOKS); // FOF_NOCONFIRMMKDIR or FOF_NO_UI
        if (!SUCCEEDED(hr))
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_SET_OPERATION_FLAGS, FALSE, 0, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_SET_OPERATION_FLAGS;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_SET_OPERATION_FLAGS, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        // Perform the operation to copy the item
        //
        pFileCopy->m_timeCopyStart = _time64(nullptr);

        hr = pfo->PerformOperations();

        pFileCopy->m_timeCopyEnd = _time64(nullptr);

        // Get the result.
        BOOL bAnyOperationsAborted = FALSE; // suc: FALSE; failed: TRUE.
        pfo->GetAnyOperationsAborted(&bAnyOperationsAborted);

        // Be canceled
        if (hr == S_OK && bAnyOperationsAborted)
        {
            int n = 0;
        }

        if (!SUCCEEDED(hr) || bAnyOperationsAborted)
        {
            if (nullptr != pFileCopy->m_fnProgressCallback)
            {
                pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_PERFORMOPERATIONS, FALSE, hr, pFileCopy->m_pProgressCallbackArg);
            }

            nRet = FC_ERR_PERFORM_OPERATIONS;
            break;
        }
        if (nullptr != pFileCopy->m_fnProgressCallback)
        {
            pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_PERFORMOPERATIONS, TRUE, 0, pFileCopy->m_pProgressCallbackArg);
        }

        //
        pFileCopy->m_timeTotalEnd = _time64(nullptr);

    } while (false);

    if (nullptr != psiTo)
    {
        psiTo->Release();
    }

    if (nullptr != psiFrom)
    {
        psiFrom->Release();
    }

    if (nullptr != pfo)
    {
        // Remove the callback
        if (bAdvise)
        {
            pfo->Unadvise(dwCookie);
        }

        pfo->Release();
    }

    CoUninitialize();

    //
    // Notify the caller that we are done， caller need call SC_CloseCopyFile to close the HANDLE.
    //
    if (nullptr != pFileCopy && nullptr != pFileCopy->m_fnProgressCallback)
    {
        pFileCopy->m_fnProgressCallback(pFile, SINK_TYPE_CFFN_FINISH_COPY, hr, nRet, pFileCopy->m_pProgressCallbackArg);
    }

    return nRet;
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
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    pFileCopy->m_pSink->Cancel();

    return 0;
}

int FC_IsCancelCopyFile(HANDLE hFileCopy, BOOL *pbCancel)
{
    if (nullptr == hFileCopy || nullptr == pbCancel)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    *pbCancel = pFileCopy->m_pSink->IsCancel();

    return 0;
}

HRESULT FC_GetCopyFileSource(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    return StringCchCopy(lpszBuffer, cchBuff, pFileCopy->m_strSrc.c_str());
}

HRESULT FC_GetCopyFileDestFile(HANDLE hFileCopy, LPWSTR lpszBuffer, int cchBuff)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    return StringCchCopy(lpszBuffer, cchBuff, pFileCopy->m_strFullNewFileName.c_str());
}

int FC_GetCopyStart(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;

    *pTime = pFileCopy->m_timeCopyStart;

    return 0;
}

int FC_GetCopyEnd(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;

    *pTime = pFileCopy->m_timeCopyEnd;

    return 0;
}

int FC_GetTotalStart(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;

    *pTime = pFileCopy->m_timeTotalStart;

    return 0;
}

int FC_GetTotalEnd(HANDLE hFileCopy, __time64_t* pTime)
{
    if (nullptr == hFileCopy)
    {
        return FC_ERR_INAVLID_PARAM;
    }

    FC_COPYFILE_HANDLE* pFileCopy = (FC_COPYFILE_HANDLE*)hFileCopy;
    *pTime = pFileCopy->m_timeTotalEnd;

    return 0;
}

int DeleteDirectoryW(LPCWSTR lpszDir)
{
    WCHAR szPath[MAX_PATH * 2] = { 0 };
    StringCchCopy(szPath, MAX_PATH * 2, lpszDir);
    // 使用 SHFileOperation 删除文件夹及其所有内容
    SHFILEOPSTRUCT fileOp;
    memset(&fileOp, 0, sizeof(fileOp));
    fileOp.fFlags = FOF_NOCONFIRMATION; // 不显示确认对话框
    fileOp.hNameMappings = NULL;
    fileOp.pFrom = szPath;
    fileOp.wFunc = FO_DELETE; // 操作类型：删除

    int result = SHFileOperation(&fileOp);

    return result;
}
