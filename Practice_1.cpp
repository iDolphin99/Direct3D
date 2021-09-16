// Practice.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
// windowsprogramming -> Our template(기본적으로 window를 띄워줌), 


// "" : local이 정의되는 header를 사용할 때 
// <> : system에서 제공해주는 header를 사용할 때 
#include "framework.h"
#include "Practice.h"

#include "windowsx.h"
#include "stdio.h"

#include "d3d11_1.h"       // _1 : feature1, 2, 3,.. function들이 안정화되어 있음 
#include <DirectXColors.h> // directX color helper

#define MAX_LOADSTRING 100


// Global Variables: 
HINSTANCE g_hInst;                                // 현재 인스턴스입니다.
WCHAR g_szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR g_szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// for D3D setting
HWND g_hWnd;                                      // window의 handler -> Direct3D의 device를 생성할 때 사용함, D3D의 device 생성 시 input paramter로 전달하기 위해 전역변수 취함 

// D3D Variables
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0; // 초기값은 설정해주지 않아도 된다
ID3D11Device* g_pd3dDevice = nullptr;                      // NULL, nullptr을 사용함으로써 그냥 null이 아닌 pointer에 쓰이는 null 임을 명시 
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;     // view : GPU에 직접적으로 할당되는 resource를 graphics pipeline의 state로 등록하는 구성 요소, resource를 직접할당하는 것이 아닌 resource에 binding된 view를 graphics pipeline에 할당하는 것으로 resource를 사용할 수 있게 됨

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
//ATOM                MyRegisterClass(HINSTANCE hInstance); -> function 통합을 위해 delete
//BOOL                InitInstance(HINSTANCE, int);         -> function 통합을 위해 delete 

// Forward declarations of functions included in this code module:
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);    // user event를 처리할 수 있는 interface function, interface function
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HRESULT InitDevice();
void CleanupDevice();
void Render();


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PRACTICE, g_szWindowClass, MAX_LOADSTRING);
    
    //MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    /*
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    MyRegisterClass 와 InitInstance function으로 나뉘어있으며 이를 편의상 하나로 합친다 
    */
    if (FAILED(InitWindow(hInstance, nCmdShow))) // FAILED : 해당 function에 대해서 실패할 경우 ~ return 값이 S_OK가 아닌 경우 
        return 0;

    if (FAILED(InitDevice())) // window handlr를 가지고 device와 GPU state를 setting하기 위한 immediatecontext, 전역 view를 여기서 설정
    {
        CleanupDevice();      // 그냥 종료하면 앞서서 할당된 memory가 그대로 남아있기 때문에 해제시키는 것 
        return 0;
    }

    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRACTICE));
    //MSG msg;

    //// Main message loop:
    //while (GetMessage(&msg, nullptr, 0, 0))
    //{
    //    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }
    //}

    // Alwats calling at evety frame!! 
    // WndProc function 통해서 별도의 스레드로 event도 호출하면서 계속해서 render를 수행할 수 있음  
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }

    CleanupDevice(); // message가 종료되면서 memory 해제 
    return (int) msg.wParam;
}

// programming 시 중간 결과 값을 console window로 확인하기 위해서는? 
// console programming의 시작은 main function 
// Linker -> subsystem -> window : 초기 템플릿 설정에서는 wWinMain이 defualt로 불림 
// main() 에서 defalut function을 부르도록 작성 
int main()
{
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWNORMAL); 
}


//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//  window를 띄우는데 그 window의 속성을 지정하는 function 
//  window의 속성을 application에 등록하는 function 
//ATOM MyRegisterClass(HINSTANCE hInstance) // ATOM : unsigned short, 16bit integer 값을 return, 성공은 1(true), 실패는 1이 아닌 값을 return 

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
//BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // PURPOSE : Registers the window class. 
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRACTICE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PRACTICE);
    wcex.lpszClassName = g_szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wcex)) // window property가 등록에 성공에 실패하면 (0인 경우)
        return E_FAIL;


    // PURPOSE: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
    g_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    // docs -> CreateWindowW macro 
    RECT rc = { 0, 0, 800, 600 }; // window resize 
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE); // window의 추가 요소 component의 크기도 반영하면서 width, heigth값을 변경하기 위해서임 
    g_hWnd = CreateWindowW(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;

    return S_OK;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_MOUSEMOVE :
        // WndProc mouse move 
        // mouse cursor's position을 읽고 console에 띄웁니다.. 
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            printf("%d, %d\n", xPos, yPos);
        }
        break; 

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//----------------------------------------------------------------------------------------------------
// Create Direct3D device and swap chain 
//----------------------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    // PURPOSE : D3D의 device를 생성하고 swap chain을 만듬 
    HRESULT hr = S_OK;

    RECT rc;                          // window의 position, 어떤 크기로 지정되어 있는가 rectangle 정보 -> size 정보를 가져올 수 있음 
    GetClientRect(g_hWnd, &rc);       // window handler로부터 rc variable을 받아옴 
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top; 

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // debug mode는 device를 debuge mode로 생성하여 program application이 진행되면서 log message를 console, output으로 확인할 수 있음 
#endif
    D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE; // hardware driver를 사용 
    D3D_FEATURE_LEVEL featureLevels[] = // 통상적인 featurelevel, aaray 작성 이유? -> D3D11CreateDevice에서 설명 참조 
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // 11 API를 사용하지만 GPU featrue level에 맞게, hardware적인 기능만 사용할 수 있게 device를 잡아줌 
    hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, // function의 parameter, feature set이 array로 들어가며 이는 feature set을 앞에서부터 시도하면서 해당 PC의 GPU가 어떤 featrue level을 지원하는지를 확인함 
        D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);                            // device의 pointer, 선택된 featurelevel, device를 직접적으로 사용하기 위한 context device pointer

    // Obtain DXGI factory from decive (since we used nullptr for pAdater above)
    // DXGI를 직접적으로 handling 할 필요 없기 때문에 해당 logic을 그대로 사용하면 됨 
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDecive = nullptr;
        hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDecive));     // QueryInterface를 통해서 해당 directX를 지원하는 Graphics hardware의 dxgi interfcae를 가져옴 
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDecive->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDecive->Release();
        }
    }
    if (FAILED(hr))
        return hr;
    // 이렇게 읽어들인 dxgi interface를 바탕으로 swap chain을 생성함 

    // Create swap chain 
    // DirectX 11.0 systems 
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;                                 // 사용할 backbuffer의 수, 2개가 될 수 있음 
    sd.BufferDesc.Width = width; 
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;           // 60frame으로 화면을 계속해서 refresh하겠다, 화면 주사율
    sd.BufferDesc.RefreshRate.Denominator = 1; 
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    // 실제 video memory에서 back - front buffer간의 swapchain mechanism에 대한 description -> hardware적인 지식을 요함 

    hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain); // g_pSwapChain : swapchain을 만들고 나면 swapchain을 기술하는 pointer를 반환함 

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut 
    dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release(); // swapchain에 대한 setting이 완료되면 반드시 relase해주어야 함 -> release를 해주지 않으면 memery leak이 발생함 
    // Direct3D API가 pointer를 받아온다 -> 해당 pointer에 대해서 memory할당이 되기 때문에 반드시 release해주어야 함 -> reference count 


    // Create a render target view 
    // swapchain을 만들었기 때문에 backbuffer를 선언해주어야 함 -> rendering에 사용되는 buffer memory는 sample의 개념이 들어가는 texture memory로 잡힘  
    ID3D11Texture2D* pBackBuffer = nullptr; // DirectX에서는 resource type -> texture와 buffer type이 있음 -> texture : sample, buffer : 일반적인 memory index 개념  
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    // Backbuffer를 graphics pipeline에 연결시켜주는 것은 pointer를 직접적으로 사용하지 않고 view interface를 통해서 수행한다
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr; 

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);


    // Setup the viewport 
    // Rendering 되는 것을 화면에 그려지기 위한 viewport setting -> 3D transform, viewing transform과 관련된 내용 
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

    return hr;
} 

//----------------------------------------------------------------------------------------------------
// Render the frame
//----------------------------------------------------------------------------------------------------
void Render()
{
    // Just cleare the backbuffer
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);
    g_pSwapChain->Present(0, 0);
    
}

//----------------------------------------------------------------------------------------------------
// Render the frame
//----------------------------------------------------------------------------------------------------
void CleanupDevice()
{
    // PURPOSE : application이 끝나도 memory에 할당된 것이 해소되지 않음 -> 명시적으로 프로그램이 종료될 때 API에서 GPU에 할당한 것들을 해제해주어야 함 
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

// Device를 만들고 DXGI interface를 Query를 불러와서 swapsystem을 만들고 
// swapsystem이 할당된 front/backbuffer를 rendertargetbuffer로 할당하고
// rendertargetbuffer에 view를 binding시켜서 view가 나중에는 graphicspipeline에서 붙게됨(g_pImmediateContext -> OMSetRenderTargets~)
// graphics pipeline에 해당 backbuffer에 rendering 결과를 넣으라고 하는 것이 OMSetRenderTargets()~
// 이렇게 설정된 것을 바탕으로 render를 수행함 

// Winproc, initDevice, Render fuction만 수정! 