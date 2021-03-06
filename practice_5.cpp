// practice_1.cpp : Defines the entry point for the application.
// Phong Lighting 실습

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
ID3D11Buffer* g_pTransformCBuffer = nullptr;
ID3D11Buffer* g_pLightCBuffer = nullptr;

ID3D11Texture2D* g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;

ID3D11RasterizerState* g_pRSState = nullptr;
ID3D11DepthStencilState* g_pDSState = nullptr;

struct TransformCBuffer
{
	Matrix mWorld;
	Matrix mView;
	Matrix mProjection;
};

struct LightCBuffer
{
	// CS에서 정의된 Light의 position 정보를 담고 있다 
	// 꼭 알아둬야 할 점! ConstantBuffer는 16byte 단위로 저장되어야 함 
	// TransformCBuffer는 Matrix이기 때문에 4x4이므로 16byte의 배수이므로 상관없었음
	// Vector3 는 12byte이므로 4byte가 더 들어가야 함 
	// 이를 위해서 int lightFlg를 넣었음 
	// 이는 나중에 directional light을 줄 지 point light을 줄 지 shader code에서 구별하기 위한 정보를 저장하는 flag 변수로 쓸 수 있음 
	Vector3 posLightCS;
	int lightFlag;
};

Matrix g_mWorld, g_mView, g_mProjection;

// This practice..
Vector3 g_pos_eye, g_pos_at, g_vec_up;

// Forward declarations of functions included in this code module:
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HRESULT InitDevice();
void CleanupDevice();
void Render();
HRESULT Recompile();

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


Vector3 ComputePosSS2WS(int x, int y, const Matrix& mView)
{
	RECT rc;
	GetWindowRect(g_hWnd, &rc);
	float w = (float)(rc.right - rc.left);
	float h = (float)(rc.bottom - rc.top);
	Vector3 pos_ps = Vector3((float)x / w * 2 - 1, -(float)y / h * 2 + 1, 0);	
	Matrix matPS2CS;
	g_mProjection.Invert(matPS2CS);												
	Matrix matCS2WS;
	mView.Invert(matCS2WS);
	Matrix matPS2WS = matPS2CS * matCS2WS; // row major
	Vector3 pos_np_ws = Vector3::Transform(pos_ps, matPS2WS);
	return pos_np_ws;
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
	static Vector3 pos_start_np_ws;
	static Vector3 pos_start_eye_ws, pos_start_at_ws, vec_start_up;
	static Matrix mView_start;
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
	// 어떤 key던 눌렸을 때 해당 logic으로 들어오게 된다. 
	case WM_KEYDOWN:
	{
		// 지우는 키에 대해서 다음의 코드를 수행하도록 한다. 
		if (wParam == VK_BACK) if (FAILED(Recompile())) printf("FAILED!!!!\n");
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		pos_start_np_ws = ComputePosSS2WS(xPos, yPos, g_mView);
		pos_start_eye_ws = g_pos_eye;
		pos_start_at_ws = g_pos_at;
		vec_start_up = g_vec_up;
		mView_start = g_mView;
	}
	break;
	case WM_MOUSEMOVE:
		// WndProc mouse move
		// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
	{
		// mouse가 move된 위치를 가지고 상대적인 위치를 계산한다 
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		// https://docs.microsoft.com/en-us/windows/win32/learnwin32/mouse-clicks
		if (wParam & MK_LBUTTON)
		{
			// To Do - Rotation logic 
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);					// 처음 press된 지점 ~ drage된 지점 vector(?)
#pragma region HW part 1
			//printf("%f, %f, %f\n", pos_start_np_ws.x, pos_start_np_ws.y, pos_start_np_ws.z);
			Vector3 vec_start_cam2np = pos_start_np_ws - pos_start_eye_ws;
			vec_start_cam2np.Normalize();
			Vector3 vec_cur_cam2np = pos_cur_np_ws - pos_start_eye_ws;
			vec_cur_cam2np.Normalize();
			float angle_rad = acosf(vec_start_cam2np.Dot(vec_cur_cam2np)) * 3.0f;
			Vector3 rot_axis = vec_start_cam2np.Cross(vec_cur_cam2np);
			if (rot_axis.LengthSquared() > 0.000001)
			{
				printf("%f\n", angle_rad);
				rot_axis.Normalize();
				Matrix matR = Matrix::CreateFromAxisAngle(rot_axis, angle_rad);

				g_pos_eye = Vector3::Transform(pos_start_eye_ws, matR);
				//g_pos_at = no change
				g_vec_up = Vector3::TransformNormal(vec_start_up, matR);
			}
#pragma endregion HW part 1
			g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
		}
		else if (wParam & MK_RBUTTON)
		{
			// To Do - Panning logic
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);
#pragma region HW part 2
			float dist_at = (pos_start_at_ws - pos_start_eye_ws).Length();
			// np : 0.01f
			Vector3 vec_diff_np = pos_cur_np_ws - pos_start_np_ws;
			float dist_diff_np = vec_diff_np.Length();
			float dist_diff = dist_diff_np / 0.01f * dist_at;
			if (dist_diff_np > 0.000001)
			{
				Vector3 vec_diff = vec_diff_np / dist_diff_np * dist_diff;
				g_pos_eye = pos_start_eye_ws - vec_diff;
				g_pos_at = pos_start_at_ws - vec_diff;
			}
#pragma endregion HW part 2
			g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
		}
	}
	break;
	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
#pragma region HW part 3
		float move_delta = zDelta > 0 ? 0.5f : -0.5f;
		Vector3 view_dir = (g_pos_at - g_pos_eye);
		view_dir.Normalize();
		g_pos_eye += move_delta * view_dir;
		g_pos_at += move_delta * view_dir;
#pragma endregion HW part 3
		g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
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

// runtime 중에도 shader를 수정했을 때 이를 window event, key event를 통해서 recompile하는 code 
// initDevice에서 shader를 file로부터 읽어와서 만드는 code를 그대로 가져와서 구현한 코드이다
// 다만, recompile하고 Blob에 저장이되고 이 Blob에 저장된 것을 가지고 vertex shader를 만들 때 globla variable로 뺀 vertex shader resource pointer를 그대로 사용하고 있음 
// -> g_pVertexShader 
// 다시 할당하기 전에 Release 안하고 하면 그전에 set된 것이 GPU memory 어딘가에 남아있음 
// 그상태에서 resource를 생성하게 되면 프로그램을 종료할 때 이 code가 cleanIpDevice에서 불리게 되는데 
// 기존에 할당되어 있던 부분은 Release가 안되게 되고 그렇게 되면 GPU memory leakege가 발생하게 된다. 
// 그렇기 때문에 set을 하기 직전에 반드시 Releaser 하는 코드를 넣어야 함 (PS도 마찬가지!) 
HRESULT Recompile()
{
	HRESULT hr = S_OK;
	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders5.hlsl", "VS_TEST", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Vertex Shader Compiler Error!!", L"Error!!", MB_OK);
		return hr;
	}

	// Create the vertex shader
	if (g_pVertexShader) g_pVertexShader->Release();
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders5.hlsl", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Pixel Shader Compiler Error!!", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	if (g_pPixelShader) g_pPixelShader->Release();
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	return hr;
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
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

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
	hr = CompileShaderFromFile(L"Shaders5.hlsl", "VS_TEST", "vs_4_0", &pVSBlob);
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
	{
		// vertex shader에 들어가는 vertex buffer의 구성이 Nor가 추가되었기 때문에 변경되어야 함 
		// color는 RGB를 사용, float 값 사용, 4byte 짜리가 4개가 쓰이고 있기 때문에 offset은 12+16=28이 되어야함 
		// Color는 사실 DXGI_FORMAT_G8G8B8A8_UNORM을 사용할 수 있음 
		// 이렇게 할 경우 각 채널이 1byte씩 color를 사용하고 color의 precision이 0~255로 각 channel을 쓰게 됨, 사실 color는 이정도 precision이면 충분함 
		// 그리고 4byte를 쓰게 되므로 "NORMAL"의 offset이 4 byte만 앞에서 color로 쓰고 있으므로 4+12 = 16이 될 것임 
		// 훨씬 효율적으로 구현할 수 있음 
		// UNORM 의 의미는 8byte로 저장이 되는데 shader에서는 float처럼 값을 사용할 수 있음 
		// shader 안에서는 똑같이 float처럼 쓰지만 실제 그 float의 precision은 8byte로 돌아간다는 의미를 가짐 
		// 이것에 대한 리포트를 테스트하여 직접 구동되는 것을 확인하면 추가 점수! 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}
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
	// Runtime에서 compile을 하고 compile된 shader를 GPU resource로 등록하고 runtime에서 GPU pipeline에 Device context가 set을 해주고 있음 
	// 이는 즉 Runtime 에서 shader code를 수정하고 프로그램이 실행되고 있는 도중에 compile하고 이를 shader resource를 만들어서 set을 하면 
	// 프로그램이 실행되고 있는 도중에도 바뀐 shader code의 결과를 볼 수 있게 됨 
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Shaders5.hlsl", "PS", "ps_4_0", &pPSBlob);
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
	struct CubeVertex
	{
		Vector3 Pos;
		Vector4 Color;
		Vector3 Nor;	// lighting이 되려면 material, object의 normal vector가 필요함 
	};

	CubeVertex vertices[] =
	{
		// cube가 원점에 있고 각각의 vertex들은 원점을 중심으로 한 정육면체로 구성되어 있기 때문에 vertex position으로부터 normal을 대각선으로 나가는 방향으로 쉽게 구할 수 있다. 
		{ Vector3(-0.5f, 0.5f, -0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f), Vector3()},
		{ Vector3(0.5f, 0.5f, -0.5f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) ,Vector3() },
		{ Vector3(0.5f, 0.5f, 0.5f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) ,Vector3() },
		{ Vector3(-0.5f, 0.5f, 0.5f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) ,Vector3() },
		{ Vector3(-0.5f, -0.5f, -0.5f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) ,Vector3() },
		{ Vector3(0.5f, -0.5f, -0.5f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) ,Vector3() },
		{ Vector3(0.5f, -0.5f, 0.5f), Vector4(1.0f, 1.0f, 1.0f, 1.0f) ,Vector3() },
		{ Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 0.0f, 1.0f) ,Vector3() },
	};
	for (int i = 0; i < 8; i++)
	{
		// pointer 개념을 써서 접근 하도록 하겠음, & syntax를 통해 해당 변수를 reference 처럼 사용할 수 있음 
		// 만약 pointer를 쓰지 않으면 call by value가 되고 해당 routine에서만 쓰이고 parameter들이 삭제되기 때문에 
		// & vtx pointer로 접근하여 vertices[]에 저장되어 있는 structure의 값을 변경할 수 있게 됨 
		CubeVertex& vtx = vertices[i];
	 	vtx.Nor = vtx.Pos;   // 엄밀히는 vtx.Pos  - Vector3(0,0,0)으로 작성한 원점으로부터의 position 방향이긴 함 
		vtx.Nor.Normalize(); // position값이 normalized된 normal vector값을 얻게 된다. 
	}

	D3D11_BUFFER_DESC bd = {}; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
	bd.Usage = D3D11_USAGE_IMMUTABLE; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage 
	bd.ByteWidth = sizeof(CubeVertex) * ARRAYSIZE(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(CubeVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	
	// Create index buffer
	WORD indices[] =
	{
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

		6,4,5,
		7,4,6,
	};
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
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(TransformCBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTransformCBuffer);
	if (FAILED(hr))
		return hr;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightCBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightCBuffer);
	if (FAILED(hr))
		return hr;

#pragma region Transform Setting
	g_mWorld = Matrix::CreateScale(10.f);

	g_pos_eye = Vector3(0.0f, 0.0f, 20.0f);
	g_pos_at = Vector3(0.0f, 0.0f, 0.0f);
	g_vec_up = Vector3(0.0f, 1.0f, 0.0f);
	g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);

	g_mProjection = Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);
#pragma endregion

#pragma region States
	D3D11_RASTERIZER_DESC descRaster;
	ZeroMemory(&descRaster, sizeof(D3D11_RASTERIZER_DESC));
	descRaster.FillMode = D3D11_FILL_SOLID;
	descRaster.CullMode = D3D11_CULL_BACK;
	descRaster.FrontCounterClockwise = true;
	descRaster.DepthBias = 0;
	descRaster.DepthBiasClamp = 0;
	descRaster.SlopeScaledDepthBias = 0;
	descRaster.DepthClipEnable = true;
	descRaster.ScissorEnable = false;
	descRaster.MultisampleEnable = false;
	descRaster.AntialiasedLineEnable = false;
	hr = g_pd3dDevice->CreateRasterizerState(&descRaster, &g_pRSState);

	D3D11_DEPTH_STENCIL_DESC descDepthStencil;
	ZeroMemory(&descDepthStencil, sizeof(D3D11_DEPTH_STENCIL_DESC));
	descDepthStencil.DepthEnable = true;
	descDepthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	descDepthStencil.StencilEnable = false;
	hr = g_pd3dDevice->CreateDepthStencilState(&descDepthStencil, &g_pDSState);
#pragma endregion


	return hr;
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{	
	TransformCBuffer cb;
	cb.mWorld = g_mWorld.Transpose(); 
	cb.mView = g_mView.Transpose();   
	cb.mProjection = g_mProjection.Transpose();
	g_pImmediateContext->UpdateSubresource(g_pTransformCBuffer, 0, nullptr, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pTransformCBuffer);
	// 해당 GPU resource를 pointing 하고 있는 direct3D pointer의 해당 application의 structure를 할당하고 있음 이를 똑같이 해주어야 함 
	LightCBuffer cbLight;
	// (0,8,0)은 WS의 점이므로 이를 CS로 Transform, 변환해주어야 한다
	cbLight.posLightCS = Vector3::Transform(Vector3(0, 8, 0), g_mView);
	// 임의로 flag값이 잘 들어가고 있는지 확인해보기 위해서 실험차 넣은 것임 
	cbLight.lightFlag = 777;
	// cbLight라는 structure가 D3D11 constant buffer가 가르키는 GPU memory에 내용물이 update됨 
	g_pImmediateContext->UpdateSubresource(g_pLightCBuffer, 0, nullptr, &cbLight, 0, 0);
	// 이렇게 update 된 CBuffer를 vertex shader의 1번 slot에 넣겠음 
	//g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pLightCBuffer);
	// phong shading을 하기 위해서는 lightConstantBuffer를 PS에서도 읽어올 수 있어야 하기 때문에 GPU resource에 올라간 lightConstantbuffer를 PS에 set해주어야 한다 
	// PS에서만 lightbuffer를 사용하면 위처럼 VSSet~은 주석처리하면 됨 
	// 나중에 phongshading말고 다른 것도 실험하기 위해서는 주석처리를 풀어야 함!! 
	// 이렇게 사소한 부분들, PS에서만 사용하므로 거기에만 set하고 VS에서는 쓰지 않으므로 set을 하지 않는 것과 같은 최적화는 memory, cashing 효율이 쌓이게 된다
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pLightCBuffer);

	g_pImmediateContext->RSSetState(g_pRSState);
	g_pImmediateContext->OMSetDepthStencilState(g_pDSState, 0);

	// Just clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	// Clear the depth buffer to 1.0 (max depth) 
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pTransformCBuffer) g_pTransformCBuffer->Release();
	if (g_pLightCBuffer) g_pLightCBuffer->Release();
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