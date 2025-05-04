#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// 全局变量
HINSTANCE g_hInst = NULL;
HWND g_hWndMain = NULL;
HTHUMBNAIL g_hThumbnail = NULL;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

wchar_t g_szTargetDisplayDevice[CCHDEVICENAME] = { 0 };
bool g_bTargetDisplayInitialized = false;
bool g_bIsExiting = false;  // 退出状态标志

RECT GetSecondaryDisplayRect()
{
    DISPLAY_DEVICEW dd = { sizeof(dd) };
    RECT rcDisplay = { 0 };

    if (!g_bTargetDisplayInitialized)
    {
        for (int i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); i++)
        {
            if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
                !(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
            {
                wcscpy_s(g_szTargetDisplayDevice, CCHDEVICENAME, dd.DeviceName);
                g_bTargetDisplayInitialized = true;
                break;
            }
        }
        if (!g_bTargetDisplayInitialized) return rcDisplay;
    }

    if (g_szTargetDisplayDevice[0] != L'\0')
    {
        DISPLAY_DEVICEW ddCheck = { sizeof(ddCheck) };
        if (!EnumDisplayDevicesW(g_szTargetDisplayDevice, 0, &ddCheck, 0) ||
            !(ddCheck.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            return rcDisplay;
        }

        DEVMODEW dm = { 0 };
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsW(g_szTargetDisplayDevice, ENUM_CURRENT_SETTINGS, &dm))
        {
            rcDisplay = {
                dm.dmPosition.x,
                dm.dmPosition.y,
                dm.dmPosition.x + static_cast<LONG>(dm.dmPelsWidth),
                dm.dmPosition.y + static_cast<LONG>(dm.dmPelsHeight)
            };
        }
    }
    return rcDisplay;
}

bool InitDirectX(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        nullptr,
        &g_pd3dDeviceContext);

    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr)) return false;

    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        SetTimer(hWnd, 1, 1000, NULL);

        HWND hWndSrc = FindWindow(L"VALORANTUnrealWindow", NULL);
        if (!hWndSrc)
        {
            MessageBox(hWnd, L"未找到目标窗口！", L"错误", MB_OK | MB_ICONERROR);
            return -1;
        }

        if (FAILED(DwmRegisterThumbnail(hWnd, hWndSrc, &g_hThumbnail)))
        {
            MessageBox(hWnd, L"缩略图注册失败！", L"错误", MB_OK | MB_ICONERROR);
            return -1;
        }

        DWM_THUMBNAIL_PROPERTIES props = { 0 };
        props.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_OPACITY;
        props.fVisible = TRUE;
        props.opacity = 140;

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        props.rcDestination = rcClient;

        DwmUpdateThumbnailProperties(g_hThumbnail, &props);
        break;
    }

    case WM_TIMER:
        if (wParam == 1 && !g_bIsExiting)
        {
            RECT rcDisplay = GetSecondaryDisplayRect();
            if (IsRectEmpty(&rcDisplay))
            {
                g_bIsExiting = true;
                KillTimer(hWnd, 1);
                MessageBox(hWnd, L"副屏已断开，程序即将退出！", L"提示", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
                break;
            }

            HWND hWndSrc = FindWindow(L"VALORANTUnrealWindow", NULL);
            if (!hWndSrc)
            {
                g_bIsExiting = true;
                KillTimer(hWnd, 1);
                MessageBox(hWnd, L"目标窗口已关闭，程序即将退出！", L"提示", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_DISPLAYCHANGE:
        if (!g_bIsExiting)
        {
            RECT rcDisplay = GetSecondaryDisplayRect();
            if (IsRectEmpty(&rcDisplay))
            {
                g_bIsExiting = true;
                KillTimer(hWnd, 1);
                MessageBox(hWnd, L"显示配置已更改，副屏不可用！", L"提示", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_SIZE:
        if (g_pSwapChain && g_pd3dDeviceContext)
        {
            if (g_pRenderTargetView)
            {
                g_pRenderTargetView->Release();
                g_pRenderTargetView = nullptr;
            }

            g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

            ID3D11Texture2D* pBackBuffer = nullptr;
            g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
            pBackBuffer->Release();

            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

            if (g_hThumbnail)
            {
                DWM_THUMBNAIL_PROPERTIES props = { 0 };
                props.dwFlags = DWM_TNP_RECTDESTINATION;
                GetClientRect(hWnd, &props.rcDestination);
                DwmUpdateThumbnailProperties(g_hThumbnail, &props);
            }
        }
        break;

    case WM_DESTROY:
        if (!g_bIsExiting)
        {
            KillTimer(hWnd, 1);
        }

        if (g_hThumbnail)
        {
            DwmUnregisterThumbnail(g_hThumbnail);
            g_hThumbnail = NULL;
        }

        if (g_pRenderTargetView)
        {
            g_pRenderTargetView->Release();
            g_pRenderTargetView = nullptr;
        }
        if (g_pSwapChain)
        {
            g_pSwapChain->Release();
            g_pSwapChain = nullptr;
        }
        if (g_pd3dDeviceContext)
        {
            g_pd3dDeviceContext->Release();
            g_pd3dDeviceContext = nullptr;
        }
        if (g_pd3dDevice)
        {
            g_pd3dDevice->Release();
            g_pd3dDevice = nullptr;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"ThumbnailDemoClass";
    return RegisterClassExW(&wcex);
}

BOOL CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    RECT rcDisplay = GetSecondaryDisplayRect();
    if (IsRectEmpty(&rcDisplay))
    {
        MessageBox(NULL, L"未找到副屏！", L"错误", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    g_hWndMain = CreateWindowW(
        L"ThumbnailDemoClass",
        L"Thumbnail Demo",
        WS_POPUP | WS_VISIBLE,
        rcDisplay.left,
        rcDisplay.top,
        rcDisplay.right - rcDisplay.left,
        rcDisplay.bottom - rcDisplay.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!g_hWndMain) return FALSE;

    if (!InitDirectX(g_hWndMain))
    {
        MessageBox(NULL, L"DirectX初始化失败！", L"错误", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);
    return TRUE;
}

void RenderFrame()
{
    if (!g_pd3dDeviceContext) return;

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
    g_pSwapChain->Present(1, 0);
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!RegisterWindowClass(hInstance))
    {
        MessageBox(NULL, L"窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!CreateMainWindow(hInstance, nCmdShow))
    {
        return 1;
    }

    HMODULE hDll = LoadLibrary(TEXT("ST.dll"));
    if (!hDll) {
        MessageBox(NULL, L"无法加载 DLL，错误代码: ", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg = { 0 };
    while (true)
    {

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            RenderFrame();
        }
    }

    return (int)msg.wParam;
}