// practice_1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Practice.h"

#include <windowsx.h>
#include <stdio.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

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
ID3D11Buffer* g_pIndexBuffer = nullptr;
ID3D11Buffer* g_pConstantBuffer = nullptr;

ID3D11Texture2D* g_pDepthStencil = nullptr; // depth buffer 별도 생성, depthstencil texture -> texture resource 
ID3D11DepthStencilView* g_pDepthStencilView = nullptr; // depthsencil texture를 reference 하는 view를 만들어 준다 -> texture resource를 depthstencilview로 쓰겠다는 reference  

ID3D11RasterizerState* g_pRSState = nullptr;	// backpace culling 내용이 담길 것임 
ID3D11DepthStencilState* g_pDSState = nullptr;  // z-test 를 통한 oculusion culling을 실행하는 내용이 담길 것임
// z-test를 setting하기 위해서 -> swap chain이 render target textrue와 연결되어 있음, dxgi 를 통해 windows handler를 얻어오고 있음 
// windows handler를 얻어와서 거기서 textrue를 쓰고 있음
// depth buffer는 화면에 그려주는 것과 직접적인 연관이 없고 순수하게 graphics 처리를 위해 사용되는 buffer이므로 별도로 생성해주어야 함 -> ID3D11Texture2D * 

struct ConstantBuffer
{
	Matrix mWorld;
	Matrix mView;
	Matrix mProjection;
};
// model / veiw / projection transform 
Matrix g_mWorld, g_mView, g_mProjection;

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
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
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

	// Create depth stencil texture
	// render target texture를 만드는 것과 비슷함 
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;						// depth와 stencil 중 stencil은 일종의 mask라고 보면 됨, 우리는 mask를 쓰지 않고 전 영역에 대해서 사용할 것임
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;		// fromat -> d32, mask를 사용하지 않기 때문에 depth만을 fully 32bit precision을 할당한다 
	descDepth.SampleDesc.Count = 1;					// multisampling, antialiacing할 때 사용하는 parameter
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;			// buffer에 값이 쓰여지기만 할 것이기 때문에 default 
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 해당 texture는 depthstencil로 binding 되어(용도로) 사용된다~ 를 의미하는 flag, 내부적으로 depthstencil용으로 특수하게 동작한다 
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil); 
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view 
	// 위 depthstencil은 resource로만 잡히기 때문에 resource를 g_p setting하기 위해 가르키는 view가 필요하다 
	// 이와 같은 setting function은 rendering function 부분에 작성하는 것이 좋다 
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	// depthsencil -> outmerge에 둔다 
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
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
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders2_ori.hlsl", "VS_TEST", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Vertex Shader Compiler Error!!", L"Error!!", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	} 

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{	// 해당 input layer에 여러 개의 position 정보를 넣어줄 수 있음 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 이것도 그냥 사용하시면 됨 
		// 여기서 InputSlot이라는 parameter가 있는데 그것은 IAstage에 총 16개의 slot을 사실은 넣을 수 있음, 하지만 우리는 0번째 slot만 사용 
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders2_ori.hlsl", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Pixel Shader Compiler Error!!", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

#pragma endregion Create Shader

#pragma region Create a cube
	struct SimpleVertex
	{	// vertex buffer의 하나의 point가 두 개의 정보를 갖게 된다 
		Vector3 Pos;
		Vector4 Color; // shader는 이게 color인지 모르니까 알려줘야 함 -> hlsl, sementacis
	};

	SimpleVertex vertices[] =
	{	// 정육면체에서 한 면에 두 개의 triangle이 들어가면 vertex만 해도 6개, 총 6*6 = 36개의 vertex가 필요함, 하지만 몇 개의 vertex는 겹치기 마련 (하나의 면당 vertex 두 개가 중복) 
		// vertex가 중복되어 사용되는 것을 피하기 위한 자료구조 -> Index buffer 
		// vertex를 공간상에 중복되지 않는 unique하게 지정하고, vertex의 순서로써 triangle을 만들 수 있다 
		// ex) 육면체, 실제로 필요한 vertex는 8개지만 어떤 순서로 정의되는지 모름, 이를 정의하기 위해 순서를 index로 표현하는 것 -> Index buffer의 역할  
		{ Vector3(-0.5f, 0.5f, -0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) }, // color는 rgba로 입력 
		{ Vector3(0.5f, 0.5f, -0.5f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(0.5f, 0.5f, 0.5f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-0.5f, 0.5f, 0.5f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ Vector3(-0.5f, -0.5f, -0.5f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ Vector3(0.5f, -0.5f, -0.5f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(0.5f, -0.5f, 0.5f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 0.0f, 1.0f) },
	};

	D3D11_BUFFER_DESC bd = {}; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
	bd.Usage = D3D11_USAGE_IMMUTABLE; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage 
	bd.ByteWidth = sizeof(SimpleVertex) * ARRAYSIZE(vertices); // ARRAYSIZE(vertices) = 8
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	
	// Create index buffer
	WORD indices[] =
	{	// 삼각형 두개가 하나의 면을 만들고 6개의 면으로 육면체를 만듬 
		3,1,0,
		2,1,3,

		0,5,4,
		1,5,0,

		3,4,7,
		0,4,3,

		1,6,5,
		2,6,1,

		2,7,6,
		3,7,2,
		// 7 2 6 
		// 7 3 2 -> 얘네만 CCW로 되어있고 나머지는 CW로 되어있던 것임~! 

		6,4,5,
		7,4,6,
	};  // 대부분의 경우 이렇게 buffer와 index buffer로 model이 이루어져 있음 
	// 마찬가지로 생성된 buffer들은 GPU memory에 잡아놔야 함 
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	// Set index buffer
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#pragma endregion

	// Create the constant buffer
	// 아래에서 만든 transform matrix를 shader에 전달하기 위해서는 constant buffer를 만들어야 함 
	bd.Usage = D3D11_USAGE_DEFAULT; // 최적화 관점에서 default로 만들고 update함수를 써서 나중에 dynamic으로 바꿔줄 수 있다고 함... 
	bd.ByteWidth = sizeof(ConstantBuffer); // struct로 세 개의 matrix를 담고 있음 
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // constant buffer는 CPU parameter가 GPU에 write되어야 한다 
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

#pragma region Transform Setting 
	g_mWorld = Matrix::CreateScale(10.f); // 4x4 matrix, xyz 모두 같은 scaling을 해줌 

	Vector3 pos_eye = Vector3(0.0f, 0.0f, 20.0f);
	Vector3 pos_at = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 vec_up = Vector3(0.0f, 1.0f, 0.0f);
	g_mView = Matrix::CreateLookAt(pos_eye, pos_at, vec_up); // view matrix 는 CreateLookAt function을 이용해서 구할 수 있음, viewing matrix 

	g_mProjection = Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f); // PIDIV2 : PI/2, 90 radian 
#pragma endregion

#pragma region States
	// resterizer state를 그리는 부분 
	D3D11_RASTERIZER_DESC descRaster;
	ZeroMemory(&descRaster, sizeof(D3D11_RASTERIZER_DESC));
	descRaster.FillMode = D3D11_FILL_SOLID;								// 채워서 그리는 것, FILL_WIREFRAME 실험해보길 바람!! 
	descRaster.CullMode = D3D11_CULL_BACK;								// BACKPACE로 culling 한다 -> BACKPACE culling을 안하고 싶다면 이곳을 CULL_NONE!! 
	descRaster.FrontCounterClockwise = true;							// front face는 ccw방향으로 한다 -> create a cube region에서 방향 그대로 만들어짐, 삼각형이 바깥 방향으로 향하는 것이 ccw방향인지 확인해보시길 바람!! 
	descRaster.DepthBias = 0;
	descRaster.DepthBiasClamp = 0;
	descRaster.SlopeScaledDepthBias = 0;
	descRaster.DepthClipEnable = true;									// depth cliping을 하겠다 -> rasterizer에서 depth가 0~1로 (view frustum이 projection space에서) 계산되는데 이때 0~1 바깥은 clip out시킨다 
	descRaster.ScissorEnable = false;									// user가 정한 rectangle 영역 외의 것은 자르겠다 -> 별도의 rectangle을 지정하지 않았기 때문에 상관없음 
	descRaster.MultisampleEnable = false;								// antialised과 관련된 issue를 다룰 때 쓰도록 하자
	descRaster.AntialiasedLineEnable = false;
	hr = g_pd3dDevice->CreateRasterizerState(&descRaster, &g_pRSState); // state -> resterizer 

	// depthstencil state를 그리는 부분 
	D3D11_DEPTH_STENCIL_DESC descDepthStencil;
	ZeroMemory(&descDepthStencil, sizeof(D3D11_DEPTH_STENCIL_DESC));
	descDepthStencil.DepthEnable = true;								// z-test를 안하고 싶다면 이곳을 false!! 
	descDepthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;		// 모든 영역(전체 픽셀)에 대해서 depth test를 하겠다 -> 부분적으로 z-depth test를 할 수 있다는 이야기(depthstencil mask) -> stencil(=mask)를 안쓰기 때문에 depth에 32bit를 모두 준 것! 
	descDepthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;			// depth가 현재 있는 pixel 값보다 더 작은 depth값이 오면 그 것을 쓰겠다 
	descDepthStencil.StencilEnable = false;								// stencil 안쓰니까 false 
	hr = g_pd3dDevice->CreateDepthStencilState(&descDepthStencil, &g_pDSState); // state -> depthstencil
	// depthstencil buffer를 만든 후 이를 가르키는 view를 만들어 outmerge stage에 set해주어야 한다! 
#pragma endregion


	return hr;
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	// 대부분의 경우 ConstantBuffer의 상태가 animation, game에서 g_p에 그대로 반영해주기 위해서 매 프레임마다 호출한다 
	// C pragramming coordinate system 은 row-major 
	// shader는 column major 
	// shader에서 쓰이기 위해서는 column major = row major.transpose 형태로 저장해야 한다 
	ConstantBuffer cb;
	cb.mWorld = g_mWorld.Transpose();
	cb.mView = g_mView.Transpose();
	cb.mProjection = g_mProjection.Transpose();
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0); // 이것을 사용하게 되면 USAGE_DEFAULT -> DYNAMIC으로 바꿔준다 
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer); // gpu에 update 된 constant buffer를 vertex shader stage에 set해준다, 여기서의 0은 register(b0)과 동일 

	// culling과 관련된 setting 
	// depth buffer에 계속해서 값이 쓰이고 있는 것임 
	// 그래서 해당 depth buffer도 rgba를 clear하는 것 처럼 1의 값으로(maximum 값-ps 기준), maximum 값으로 setting해주는 것  
	g_pImmediateContext->RSSetState(g_pRSState);
	g_pImmediateContext->OMSetDepthStencilState(g_pDSState, 0);

	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	// Clear the depth buffer to 1.0 (max depth)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->DrawIndexed(36, 0, 0); // index가 총 36개다 -> 한면에 index = 6 , 총 6면이여서 -> 순서대로 그려지고 있을 때 덧그려짐으로써 culling이 되지 않고 있음
	// culling을 해결하기 위해 backspace만 culling하거나 
	// z-test culling, 앞에 있는 것만 그려지도록 하거나 

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();

	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pRSState) g_pRSState->Release();
	if (g_pDSState) g_pDSState->Release();

	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}