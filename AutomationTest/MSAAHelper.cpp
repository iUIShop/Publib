#include "MSAAHelper.h"
#include <oleacc.h>
#include <atlbase.h>

CMSAAHelper::CMSAAHelper()
{

}

CMSAAHelper::~CMSAAHelper()
{

}

int CMSAAHelper::Init(HWND hHost)
{
	m_hWndHost = hHost;
	return 0;
}

void PrintAccessibleChildren(IAccessible* pParent, int depth = 0)
{
    long lCount = 0;
    HRESULT hr = pParent->get_accChildCount(&lCount);

    VARIANT varChild;
    varChild.vt = VT_I4;

    // 假设子元素数量不超过某个限制（实际上需要动态确定，但这里简化处理）  
    const int MAX_CHILDREN = 100;
    for (varChild.lVal = CHILDID_SELF; varChild.lVal < CHILDID_SELF + MAX_CHILDREN; ++varChild.lVal)
    {
        IAccessible* pChild = nullptr;
        HRESULT hr = pParent->get_accChild(varChild, (IDispatch **)&pChild);

        if (hr == S_OK && pChild != nullptr)
        {
            // 递归打印子元素的名称  
            BSTR bstrName = nullptr;
            hr = pChild->get_accName(varChild, &bstrName);
            if (SUCCEEDED(hr) && bstrName != nullptr)
            {
                for (int i = 0; i < depth; ++i)
                {
                    //std::wcout << L"  "; // 使用缩进表示层级  
                }
                //std::wcout << L"Child Name: " << bstrName << std::endl;
                SysFreeString(bstrName);

                // 递归遍历子元素的子元素  
                PrintAccessibleChildren(pChild, depth + 1);
            }

            pChild->Release();
        }
        else if (hr == S_FALSE)
        {
            // 没有更多子元素  
            break;
        }
    }
}

void PrintBSTR(BSTR bstr) {
    if (bstr) {
        //std::wcout << static_cast<wchar_t*>(bstr) << std::endl;
        SysFreeString(bstr);
    }
}

//void TraverseAccessibleChildren(IAccessible* pParent, int indent = 0) {
//    long childCount = 0;
//    HRESULT hr = pParent->get_childCount(&childCount);
//    if (FAILED(hr)) {
//        std::cerr << "Failed to get child count. Error: " << hr << std::endl;
//        return;
//    }
//
//    for (long i = 0; i < childCount; ++i) {
//        VARIANT varChild;
//        varChild.vt = VT_I4;
//        varChild.lVal = CHILDID_SELF + i + 1; // Starting from CHILDID_SELF + 1  
//
//        IDispatch* pDispatch = nullptr;
//        hr = pParent->get_accChild(varChild, &pDispatch);
//        if (FAILED(hr)) {
//            std::cerr << "Failed to get child. Error: " << hr << std::endl;
//            continue;
//        }
//
//        IAccessible* pChild = nullptr;
//        hr = pDispatch->QueryInterface(IID_IAccessible, reinterpret_cast<void**>(&pChild));
//        if (FAILED(hr)) {
//            std::cerr << "Failed to QI for IAccessible. Error: " << hr << std::endl;
//            pDispatch->Release();
//            continue;
//        }
//
//        // Print indentation for visual hierarchy  
//        for (int j = 0; j < indent; ++j) {
//            std::wcout << "  ";
//        }
//
//        BSTR name;
//        hr = pChild->get_accName(CHILDID_SELF, &name);
//        if (SUCCEEDED(hr)) {
//            std::wcout << "Element: ";
//            PrintBSTR(name);
//        }
//        else {
//            std::wcout << "Element (no name): ";
//        }
//
//        // Recursively traverse the children of the current child  
//        TraverseAccessibleChildren(pChild, indent + 1);
//
//        pChild->Release();
//        pDispatch->Release();
//    }
//}

int CMSAAHelper::BuildUITree()
{
    // 初始化 COM 库
    CoInitialize(NULL);

    // Your code here
    IAccessible* pAcc = nullptr;
    HRESULT hr = AccessibleObjectFromWindow(m_hWndHost, OBJID_CLIENT, IID_IAccessible, (void**)&pAcc);
    if (SUCCEEDED(hr) && pAcc)
    {
        // 成功获取 IAccessible 接口
    }
    else
    {
        // 处理错误
        return -1;
    }

    //POINT pt;
    //pt.x = 1363;
    //pt.y = 17;
    //VARIANT vt;
    //hr = AccessibleObjectFromPoint(pt, &pAcc, &vt);

    VARIANT varChild;
    VariantInit(&varChild);
    varChild.vt = VT_I4;
    varChild.lVal = CHILDID_SELF;

    CComBSTR bstrName = nullptr;
    hr = pAcc->get_accName(varChild, &bstrName);

    PrintAccessibleChildren(pAcc);

    long childCount = 0;
    hr = pAcc->get_accChildCount(&childCount);  // 获取子元素数量
    if (SUCCEEDED(hr) && childCount > 0)
    {
        for (long i = 1; i <= childCount; i++)
        {
            VARIANT varChild;
            VariantInit(&varChild);
            varChild.vt = VT_I4;
            varChild.lVal = CHILDID_SELF;

            // 获取子元素
            IDispatch* pDispChild = nullptr;
            hr = pAcc->get_accChild(varChild, &pDispChild);
            if (SUCCEEDED(hr) && pDispChild)
            {
                IAccessible* pAccChild = nullptr;
                hr = pDispChild->QueryInterface(IID_IAccessible, (void**)&pAccChild);
                if (SUCCEEDED(hr) && pAccChild)
                {
                    // 可以操作子元素 pAccChild
                    BSTR name;
                    pAccChild->get_accName(varChild, &name);
                    // 打印子元素的名字
                    //wprintf(L"Child %d Name: %s\n", i, name);
                    SysFreeString(name);
                    pAccChild->Release();
                }
                pDispChild->Release();
            }
        }
    }

    // 结束时释放 COM 库
    CoUninitialize();
	return 0;
}
