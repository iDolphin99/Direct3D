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

using namespace DirectX::SimpleMath;			  // directxmath�� ����ϱ� ���ؼ� �ٿ��߸� �ϴ� namespace�� �̸� �����صθ� �������ʹ� �ٷ� ����� �� �ִ�
//DirectX::SimpleMath::Vector3 test;			  // �̷��� �ۼ��� �ʿ䰡 ���� 
//Vector3 test1;								  // �̰� �Ƹ� float������ x,y,z�� ���� vector3�� �� (wrapper structure)

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

// 4���� �߰����� g_p�� ���õ� resource 
// This practice..
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr; 
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;          // 1. ���� InitDevice���� ����� 

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

	// 1. InitDevice() : Device�� device context ����, ù ������ �ϳ��� device�� �ϳ� �̻��� device context�� ������ �Ѵ�
	// ImmediateContext�� ù ��°�� �����Ǵ� device context�� background�� �ƴ� foreground���� ���������� GPU���� ����ϱ� ���� ���Ǵ� pointer�̴� 
	hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	// DXGI : Direct X Graphics Infrastructure ��ݽü� 
	// 2. DXGI interface :D3D device�� ���� ������ �� ���� 
	// �׸��� DXGI interface�� ���ؼ� swap chain system�� ������ �� ���� 
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

	// ������ dxgi interface���� ������ swap chain�� d3dDevcie�� ����Ǿ� �ִ� 
	hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	// Create a render target view
	// 3. ������ swap chain���� frontbuffer(screenbuffer), backbuffer(rendering �� ����� �׷��� buffer)�� ������ �ִ�. 
	// buffer ��ü�� graphics pipeline���� render target���� ������ �ϹǷ� resource�� ���� pointer, (in directX) view�� �����Ѵ�. 
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
	// ���Ŀ� viewport ������ ���ؼ� �����ϰ���
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

// pragma : coding ������ grouping �ϴ� �� 
#pragma region Create a triangle
	// vertex buffer�� ����� triangle�� �����Ѵ�. 
	
	// position���� ���� vertex buffer 
	struct SimpleVertex { 
		Vector3 Pos;	// DirectX::SimpleMath::Vector3::Pos;
						// vertexbuffer���� position�Ӹ� �ƴ϶� pointer������ color, normal, coordinate, ... pointer�� ���� ������ ���� �� ���� 
	};

	// triangle�� array�� ���� 
	SimpleVertex vertices[] = // ���� element�� ������ �°� static�ϰ� array�� ���� ������ 
	{
		Vector3(0.0f, 0.5f, 0.5f),
		Vector3(0.0f, -0.5f, 0.5f),
		Vector3(-0.0f, -0.5f, 0.5f),
	}; // you can get vertices[0] = Vector3's first element

	// direct3D device���� buffer resource�� ������ �ϰ� �̿� ���� description�� �����ؾ� �Ѵ�. (resource ����ϱ� ���� description ������ �ʼ� routine)
	D3D11_BUFFER_DESC bd = {};				  // buffer description�� ���ؼ��� google it 
	bd.Usage = D3D11_USAGE_IMMUTABLE;		  // �츮�� ���� ���� ���� GPU�� ���ؼ� ������ ���� ���� (write �ϸ� readable��! write�� �� ����� �����̹Ƿ�) 
	//bd.Usage = D3D11_USAGE_DEFAULT;         // ���� �Ϲ����� Usage, �ش� resource�� GPU�� ���� ���̰� ������ �� �ִ� -> g_p������ �а� �������� listription�� �������� ������ ������  
	bd.ByteWidth = sizeof(SimpleVertex) * 3;  // buffer size�� byte ������ ���ΰ� 
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;  // �ش� buffer�� vertex buffer�̴� 
	bd.CPUAccessFlags = 0;					  // IMMUTABLE�� CPU Access�� ������� �����Ƿ� 0 

	// IMMUTABLE �̱� ������ vertex buffer ���� �� initialize buffer�� ���� ���� ��, �̸� �ֱ� ���� �ڵ� 
	D3D11_SUBRESOURCE_DATA InitData = {}; 
	InitData.pSysMem = vertices;

	// ��� resource�� ������ device pointer�κ��� ��ɾ ������ �� �� ����
	// g_pVertexBuffer�� �ش� GPU memory�� ��Ÿ���� CPU ���� memory�� ������ �� 
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer 
	// ������ vertex buffer�� GPU resource�󿡸� �ֱ� ������ �̸� g_p�� mapping �ؾ��Ѵ�. 
	// g_p mapping �ϴ� ���� ������ command�� �� �� �ְ� �̴� ��� Context device�� �۾��Ѵ�
	// device -> resource ����, ���� / context -> g_p mapping, setting, command�� �ְų� 
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// Set primitive topology 
	// primitive topology�� �˷��־�� ��
	// D3D11_TOPOLOGY�� �������� �����ϴµ� ���� TRIANGLELIST�� vertex data�� triangle�� �޾Ƶ��δٴ� �� 
	// buffer�� 9���� ������ �ﰢ���� ��Ÿ���� index 9���� �� 3���� �ﰢ���� ����� 
	// TRIANGLESTRIP �� �ﰢ�� ���� �ﰢ���� �پ ����� ���... �� �Ⱦ�! 
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
// program�� ����� �� ȣ���, device ���� �� fail�Ҷ��� ȣ���ϰ� �� 
// resource�� ���õ�, pointer�� ������ ��� �⺻������ GPU �޸𸮿� ������ ������ Release���־�� �� (Reference count)
// get, create�� �Ǹ� �ش� resource�� reference count�� ��� �����ϰ� ��
// ���� get, create�� ������ pointer�� ���̻� ���� �ʴ´ٸ� �ݵ�� Release���־�� reference count�� �پ��� �ᱹ 0�� �Ǿ� GPU driver�� �����Ѵ�
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState(); // ���� �� ��� state�� null�� �ٲپ��ش� (convention)

	if (g_pRenderTargetView) g_pRenderTargetView->Release(); // ���� ���� null���� Ȯ����(if, null�� �ƴ� ��츸) Release -> safe Release, safe Delete ����̶� �� 
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}