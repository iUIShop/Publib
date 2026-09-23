// UIAProviderDemo.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "UIAProviderDemo.h"
#include "UIA_Control/CustomControl.h"
#include "UIA_Provider/UIAProviders.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
Canvas2* g_pRootCanvas = nullptr;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_UIAPROVIDERDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UIAPROVIDERDEMO));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

// Helper function.
Canvas* GetRoot(HWND hwnd)
{
	return reinterpret_cast<Canvas*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

// Handles window messages for the HWND that contains the custom control.
//
LRESULT CALLBACK CanvasWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		// Create the control object.
		Canvas* pCanvas = new (std::nothrow) Canvas(hwnd);

		// Save the class instance as window data so that its members 
		// can be accessed from within this function.
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCanvas));
		break;
	}

	case WM_DESTROY:
	{
		// Destroy the control so interfaces are released.
		Canvas* pCanvas = GetRoot(hwnd);
		delete pCanvas;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);

		PostQuitMessage(0);

		break;
	}

	case WM_GETOBJECT:
	{
		// Register the control with UI Automation.
		// If the lParam matches the RootObjectId, send back the canvas provider.
		if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId))
		{
			// Get the control.
			Canvas* pCanvas = GetRoot(hwnd);
			if (nullptr != pCanvas)
			{
				// Return its associated UI Automation provider.
				LRESULT lresult = UiaReturnRawElementProvider(
					hwnd, wParam, lParam, pCanvas->GetProvider());
				return lresult;
			}
		}
		return 0;
	}

	case WM_PAINT:
	{
		// Retrieve the control.
		Canvas* pCanvas = GetRoot(hwnd);

		// Set up graphics context.
		PAINTSTRUCT paintStruct;
		HDC hdc = BeginPaint(hwnd, &paintStruct);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		// Save the context.
		HGDIOBJ oldHgdi = SelectObject(hdc, GetStockObject(BLACK_PEN));

		// Draw items.
		// Create and select a null pen so the rectangle isn't outlined.
		HPEN nullPen = CreatePen(PS_NULL, 1, RGB(0, 0, 0));
		SelectObject(hdc, nullPen);

		// Erase the whole window.
		Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right,
			clientRect.bottom);

		// Set transparency for text.
		SetBkMode(hdc, TRANSPARENT);

		// Create brushes
		HBRUSH unfocusedFillBrush = GetSysColorBrush(COLOR_BTNFACE);
		HBRUSH focusedFillBrush = GetSysColorBrush(COLOR_HIGHLIGHT);

		HBRUSH onlineFillBrush = CreateSolidBrush(RGB(0, 192, 0));  // Green.
		HBRUSH offlineFillBrush = CreateSolidBrush(RGB(255, 0, 0)); // Red.

		WLWnd *pChild = pCanvas->m_pChild;
		while (nullptr != pChild)
		{
			pChild->OnDraw(hdc);

			pChild = pChild->m_pNext;
		}

		EndPaint(hwnd, &paintStruct);
		// Restore context.
		SelectObject(hdc, oldHgdi);
		// Clean brushes.
		DeleteObject(nullPen);
		DeleteObject(focusedFillBrush);
		DeleteObject(unfocusedFillBrush);
		DeleteObject(onlineFillBrush);
		DeleteObject(offlineFillBrush);
		break;
	}

	case WM_SETFOCUS:
	{
		Canvas* pCanvas = GetRoot(hwnd);
		if (pCanvas != NULL)
		{
			pCanvas->SetIsFocused(true);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;
	}
	case WM_KILLFOCUS:
	{
		Canvas* pCanvas = GetRoot(hwnd);
		pCanvas->SetIsFocused(false);
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}

	case WM_GETDLGCODE:
	{
		// Trap arrow keys.
		return DLGC_WANTARROWS | DLGC_WANTCHARS;
		break;
	}

	}  // switch (message)

	return DefWindowProc(hwnd, message, wParam, lParam);
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = CanvasWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UIAPROVIDERDEMO));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_UIAPROVIDERDEMO);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		1000, 500, 500, 500, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}
