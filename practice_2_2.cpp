// practice_1.cpp : Defines the entry point for the application.
// 실습 7_2 

#include "framework.h"
#include "Practice.h"

#include <windowsx.h>
#include <stdio.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>  // shader를 compile하는 function을 제공한다 

#include <directxmath.h>
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;

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

// This practice..
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;

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

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	// runtime 중에 file을 읽어들여서 runtime 중에 compile하고 compile된 것을 shader에 등록한다 
	HRESULT hr = S_OK;

	// dwshaderflags : 어떤 방식으로 shader를 compile할 것인가 -> STRICTNESS, 가장 일반적인 mode -> 성능을 최대로 낼 필요 없이 debugging 정보를 tracing할 수 있게끔 동작하게 하는 mode 
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// debug일 때는 해당 routine 으로 들어온다 -> release하면 undebug 상태가 되므로 빠져나감 
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Errorblob : compile은 CPU에서 수행되고 그것이 blob에 pointer로 저장된다, 임의의 temporal data structure, 온갖 것이 들어갈 수 있음, GPU에 저장되거나 최소한 GPU에 직접 접근이 가능한 CPU memory상에 저장된다는 것은 분명함 
	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, // DirectX의 SDK function
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	// szFileName : shader code가 정의되어 있는 file을 읽으면, szEntryPoint : file 내의 어떤 function을 어떤 shader model로 compile하겠다 
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
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

	hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
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

	hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	// Create a render target view
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
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

#pragma region Create Shader
	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr; // blob : vertex shader를 저장하는 blob, compile된 결과를 저장한다  
	// vertex shader 는 각종 transform을 거치는 곳임 하지만 바로 return 하고 있음 (shaders.hlsl 파일 참고) -> 바로 rasterizer로 보내고 있음 
	// 이는 즉 모든 transform이 identity matrix로 들어왔따는 것을 의미한다 -> object space = projection space  
	hr = CompileShaderFromFile(L"Shaders.hlsl", "VS_TEST", "vs_4_0", &pVSBlob);
	// "VS_TEST" : entry
	// "vs_4_0" : 해당 shader를 vertex shader로 compile하겠다, shader model은 여러가지가 있는데 4.0을 사용하겠다 
	// pVSBlob : 결과를 여기에 저장하겠다 
	if (FAILED(hr)) // 항상 해두면 좋다 
	{
		MessageBox(nullptr, L"Vertex Shader Compiler Error!!", L"Error!!", MB_OK); // L? 해당 project가 multibite project여서 여기서 쓰이는 영어가 multibite로 쓰이는 string을 나타낼 때 쓴다, 16bit character set을 쓰겠다 
		return hr;
	}

	// Create the vertex shader
	// shader도 하나의 resource로 보기 때문에 GPU memory에 shader resource를 만들고 이를 vertex shader에 mapping 해야 한다 => 걍 그대로 가져다 쓴다! 
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	// vertex buffer가 input assembler에 들어간 후 이것이 vertex shader에 들어가게 됨 
	// 그래서 vertex shader가 어떤 값을 받는지 명시적으로 선언해주어야 한다 
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // input element는 "position"만, vector3 - float정의된 format으로 사용하고 이것이 position에 들어간다, vertex_data의 input이다 
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	// 이번에는 vertex buffer -> assembler 가 아니라 그 다음 단계인 assembler -> vertex shader 의 input에 대한 선언을 해준다 
	// 예를 들면 실제로 simplevertex에 대해서 position 하나만 저장하는 것이 아니라 normal, texture 등 다양한 정보가 들어갈 수 있음 (여러개의 channel로 저장되어 있다)
	// 그럼에도 불구하고 input_layer로 position만 받게 한다면 vertex buffer에서 assebler로 들어갈 때 다양한 channel 정보가 들어가더라도 poistion 정보만 layer로 쓰게 지정해놔야 한다 
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	// 그래서 위에서 지정한 vertex layer를 input assembler 에 지정해주면 된다 
	// 다양한 종류의 input layer를 가질 수 있기 때문에 이를 효과적으로 동작하기 위해서는 VertexLayout이라는 명시적인 자료구조를 가지고 assemlber에 선언해준다 
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders.hlsl", "PS", "ps_4_0", &pPSBlob); // "PS" : entry, "ps_4_0" : pixel shader model로 compile하겠다 
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Pixel Shader Compiler Error!!", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	// compile된 결과를 바탕으로 GPU memory에 resource를 만들어둔다 -> 만들었지 vertex shader에 setting한 것은 아님! 
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

#pragma endregion Create Shader

#pragma region Create a triangle
	struct SimpleVertex
	{
		Vector3 Pos;
	};

	// 구현한 삼각형 
	SimpleVertex vertices[] = 
	{
		Vector3(0.0f, 0.5f, 0.5f),
		Vector3(0.5f, -0.5f, 0.5f),
		Vector3(-0.5f, -0.5f, 0.5f),
	};

	// USAGE_IMMUTABLE -> 가장 빠른 GPU memory access가 가능하도록 하는, read만 허용
	D3D11_BUFFER_DESC bd = {};			// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
	bd.Usage = D3D11_USAGE_IMMUTABLE;	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage 
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	// usage가 immutable한(gpu에 read만 허용하는, 가장 빠른 memory access 성능 가능) buffer를 만들고 
	// buffer를 생성할 때는(resource) device으로 생성하고 초기 initdata를 할당해야 한다  
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer); // GPU memory에 삼각형 cpu상의 내용이(vertex buffer memory) 잡히게 된다 
	if (FAILED(hr))
		return hr;
	 
	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset); // 설정된 vertexbuffer를 inputassembler, gpu g_p에 contextdevcie를 통해 등록한다 

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // vertex buffer에 등록되어 있는 primitive가 TRIANGLELIST로 정의되어 있다
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

	// Render a triangle
	// 생성된 buffer(in GPU memory)를 shader에 setting 해줍니다 (?)
	// vertex buffer를 gpu에 만들고 input-assembler에 넣었음 -> vertex buffer가 triangle list(primitive 정보)로 저장되어 있다라는 정보까지 넣음 
	// vertex shader, pixel shader를 GPU memory resource로 생성 -> vertex shader같은 경우는 position만 input layout으로 받는다라고 했음 -> 이를 assembler에 저장하고 set했음 
	// 이렇게 g_p setting 된 것을 draw 하면 쭈욱 output-merge stage까지 나오게 됨 
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->Draw(3, 0); // vertex 개수가 3, vertexcount, Directx11 Draw google it 

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();

	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}