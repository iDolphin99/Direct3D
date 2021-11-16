// practice_1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Practice.h"

#include <windowsx.h>
#include <stdio.h>

#include <d3d11_1.h>
#include <directxcolors.h>

#include <directxmath.h>
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;			  // directxmath를 사용하기 위해서 붙여야만 하는 namespace를 미리 선언해두면 다음부터는 바로 사용할 수 있다
//DirectX::SimpleMath::Vector3 test;			  // 이렇게 작성할 필요가 없음 
//Vector3 test1;								  // 이건 아마 float형태의 x,y,z를 갖는 vector3일 듯 (wrapper structure)

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE g_hInst;                                // current instance
WCHAR g_szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR g_szWindowClass[MAX_LOADSTRING];            // the main window class name

// for D3D setting
HWND g_hWnd;

// D3D Variables
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

// 4개의 추가적인 g_p과 관련된 resource 
// This practice..
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr; 
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;          // 1. 먼저 InitDevice에서 만들거 

// Forward declarations of functions included in this code module:
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
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

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_PRACTICE, g_szWindowClass, MAX_LOADSTRING);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRACTICE1));
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

	CleanupDevice();
	return (int)msg.wParam;
}

int main()
{
	return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWNORMAL);
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	//  PURPOSE: Registers the window class.
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

	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	g_hInst = hInstance; // Store instance handle in our global variable

	// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindoww
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindowW(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd)
	{
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	return S_OK;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
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
	case WM_MOUSEMOVE:
		// WndProc mouse move
		// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
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
		// TODO: Add any drawing code that uses hdc here...
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

// Message handler for about box.
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
#include <iostream>
//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

#pragma region Previous class
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// 1. InitDevice() : Device와 device context 생성, 첫 생성시 하나의 device는 하나 이상의 device context를 가져야 한다
	// ImmediateContext는 첫 번째로 생성되는 device context로 background가 아닌 foreground에서 직접적으로 GPU에게 명령하기 위해 사용되는 pointer이다 
	hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	// DXGI : Direct X Graphics Infrastructure 기반시설 
	// 2. DXGI interface :D3D device를 통해 가져올 수 있음 
	// 그리고 DXGI interface를 통해서 swap chain system을 가져올 수 있음 
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	// DirectX 11.0 systems
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	// 생성된 dxgi interface에서 생성한 swap chain은 d3dDevcie와 연결되어 있다 
	hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	// Create a render target view
	// 3. 생성된 swap chain에는 frontbuffer(screenbuffer), backbuffer(rendering 된 결과가 그려질 buffer)를 가지고 있다. 
	// buffer 자체는 graphics pipeline에서 render target으로 잡혀야 하므로 resource에 대한 pointer, (in directX) view를 생성한다. 
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
#pragma endregion 
	
	// Setup the viewport
	// 추후에 viewport 설정에 대해서 설명하겠음
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

// pragma : coding 영역을 grouping 하는 것 
#pragma region Create a triangle
	// vertex buffer를 만들고 triangle을 저장한다. 
	
	// position값만 갖는 vertex buffer 
	struct SimpleVertex { 
		Vector3 Pos;	// DirectX::SimpleMath::Vector3::Pos;
						// vertexbuffer에는 position뿐만 아니라 pointer마다의 color, normal, coordinate, ... pointer에 대한 정보를 가질 수 있음 
	};

	// triangle은 array로 지정 
	SimpleVertex vertices[] = // 들어가는 element의 갯수에 맞게 static하게 array를 만들어서 저장함 
	{
		Vector3(0.0f, 0.5f, 0.5f),
		Vector3(0.0f, -0.5f, 0.5f),
		Vector3(-0.0f, -0.5f, 0.5f),
	}; // you can get vertices[0] = Vector3's first element

	// direct3D device에서 buffer resource를 만들어야 하고 이에 대한 description을 지정해야 한다. (resource 사용하기 위한 description 지정은 필수 routine)
	D3D11_BUFFER_DESC bd = {};				  // buffer description에 대해서는 google it 
	bd.Usage = D3D11_USAGE_IMMUTABLE;		  // 우리는 최초 생성 이후 GPU에 의해서 쓰여질 일이 없음 (write 하면 readable함! write가 더 어려운 연산이므로) 
	//bd.Usage = D3D11_USAGE_DEFAULT;         // 가장 일반적인 Usage, 해당 resource는 GPU에 의해 쓰이고 읽혀질 수 있다 -> g_p에서는 읽고 쓰여지는 listription이 많을수록 성능이 빠르다  
	bd.ByteWidth = sizeof(SimpleVertex) * 3;  // buffer size가 byte 단위로 얼마인가 
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // 해당 buffer는 vertex buffer이다 
	bd.CPUAccessFlags = 0;					  // IMMUTABLE은 CPU Access를 허용하지 않으므로 0 

	// IMMUTABLE 이기 때문에 vertex buffer 생성 시 initialize buffer가 같이 들어가게 됨, 이를 넣기 위한 코드 
	D3D11_SUBRESOURCE_DATA InitData = {}; 
	InitData.pSysMem = vertices;

	// 모든 resource의 생성은 device pointer로부터 명령어를 가져다 쓸 수 있음
	// g_pVertexBuffer에 해당 GPU memory를 나타내는 CPU 상의 memory가 잡히게 됨 
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer 
	// 생성된 vertex buffer는 GPU resource상에만 있기 때문에 이를 g_p에 mapping 해야한다. 
	// g_p mapping 하는 것은 일종의 command로 볼 수 있고 이는 모두 Context device가 작업한다
	// device -> resource 생성, 관리 / context -> g_p mapping, setting, command를 주거나 
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Set primitive topology 
	// primitive topology를 알려주어야 함
	// D3D11_TOPOLOGY는 여러개가 존재하는데 그중 TRIANGLELIST는 vertex data를 triangle로 받아들인다는 것 
	// buffer가 9개가 들어오면 삼각형을 나타내는 index 9개가 총 3개의 삼각형을 만든다 
	// TRIANGLESTRIP 은 삼각형 옆에 삼각형이 붙어서 만드는 방식... 잘 안씀! 
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma endregion  

	return hr;
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
// program이 종료될 때 호출됨, device 세팅 시 fail할때도 호출하게 됨 
// resource와 관련된, pointer를 가져운 경우 기본적으로 GPU 메모리에 잡히기 때문에 Release해주어야 함 (Reference count)
// get, create가 되면 해당 resource의 reference count가 계속 증가하게 됨
// 만약 get, create로 가져온 pointer를 더이상 쓰지 않는다면 반드시 Release해주어야 reference count가 줄어들고 결국 0이 되어 GPU driver가 해제한다
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState(); // 해제 전 모든 state를 null로 바꾸어준다 (convention)

	if (g_pRenderTargetView) g_pRenderTargetView->Release(); // 해제 전에 null인지 확인후(if, null이 아닌 경우만) Release -> safe Release, safe Delete 방식이라 함 
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}