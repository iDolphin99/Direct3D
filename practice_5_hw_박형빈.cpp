// practice_1.cpp : Defines the entry point for the application.
// 

#include "framework.h"
#include "Practice.h"

#include <windowsx.h>
#include <stdio.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

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
ID3D11Buffer* g_pMtCBuffer = nullptr;

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

// HW2.4) Register hard coding variables to Constant Buffers
struct LightCBuffer
{
	Vector3 posLightCS;
	int lightFlag;

	Vector3 lightColor;
	int dummy1;
};
struct MtCBuffer
{
	Vector3 mtcAmbient;
	float shine;

	Vector3 mtcDiffuse;
	int dummy2;
	Vector3 mtcSpec;
	int dummy3;
};

Matrix g_mWorld, g_mView, g_mProjection;

// This practice..
Vector3 g_pos_eye, g_pos_at, g_vec_up;
int g_lightFlag;

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
	static Matrix mView_start, mWorld_start;
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
	case WM_KEYDOWN:
	{
		if (wParam == VK_BACK) if (FAILED(Recompile())) printf("FAILED!!!!\n");

		// HW2.5) Add an option to the scene property : Paralle lighting or Point lighting
		if (wParam == VK_NUMPAD1 or wParam == 0x31) g_lightFlag = 1;
		if (wParam == VK_NUMPAD2 or wParam == 0x32) g_lightFlag = 2;
	}
	break;
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
		mWorld_start = g_mWorld;
	}
	break;
	case WM_MOUSEMOVE:
	{
		// WndProc mouse move
		// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		if (wParam & MK_LBUTTON) {
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);

			Vector3 vec_start_cam2np = pos_start_np_ws - pos_start_eye_ws;
			vec_start_cam2np.Normalize();
			Vector3 vec_cur_cam2np = pos_cur_np_ws - pos_start_eye_ws;
			vec_cur_cam2np.Normalize();

			float angle_rad = acosf(vec_start_cam2np.Dot(vec_cur_cam2np)) * 3.0f;
			Vector3 rot_axis = vec_start_cam2np.Cross(vec_cur_cam2np);

			if (rot_axis.LengthSquared() > 0.000001) {
				printf("%f\n", angle_rad);
				rot_axis.Normalize();

				// HW1) Control your camera and models - Rotation
				if (wParam & MK_CONTROL) {
					printf("keydown\n");
					Matrix matR = Matrix::CreateFromAxisAngle(rot_axis, -angle_rad);
					g_mWorld = matR * mWorld_start;
				}
				else {
					Matrix matR = Matrix::CreateFromAxisAngle(rot_axis, angle_rad);
					g_pos_eye = Vector3::Transform(pos_start_eye_ws, matR);
					g_vec_up = Vector3::TransformNormal(vec_start_up, matR);
					g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
				}
			}
		}

		else if (wParam & MK_RBUTTON) {
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);
			float dist_at = (pos_start_at_ws - pos_start_eye_ws).Length();
			// np : 0.01f
			Vector3 vec_diff_np = pos_cur_np_ws - pos_start_np_ws;
			float dist_diff_np = vec_diff_np.Length();
			float dist_diff = dist_diff_np / 0.01f * dist_at;

			if (dist_diff_np > 0.000001) {
				Vector3 vec_diff = vec_diff_np / dist_diff_np * dist_diff;
				printf("%f, %f, %f\n", pos_start_np_ws.x, pos_start_np_ws.y, pos_start_np_ws.z);

				// HW1) Control your camera and models - Panning
				if (wParam & MK_CONTROL) {
					Matrix matP = Matrix::CreateTranslation(vec_diff);
					g_mWorld = mWorld_start * matP;
					printf("keydown \n");
				}
				else {
					g_pos_eye = pos_start_eye_ws - vec_diff;
					g_pos_at = pos_start_at_ws - vec_diff;
					g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
				}
			}
		}
	}
	break;
	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		float move_delta = zDelta > 0 ? 0.5f : -0.5f;

		Vector3 view_dir = (g_pos_at - g_pos_eye);
		view_dir.Normalize();

		// HW1) Control your camera and models - Zoom 
		if (wParam & MK_CONTROL) {
			move_delta = zDelta > 0 ? 1.05f : 0.95f;
			Matrix matZ = Matrix::CreateScale(move_delta);
			g_mWorld = g_mWorld * matZ;
			printf("keydown \n");
		}
		else {
			g_pos_eye += move_delta * view_dir;
			g_pos_at += move_delta * view_dir;
			g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
		}
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
		// HW2.1) Replace Vertex Color Attribute 
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
		Vector3 Nor;
	};

	CubeVertex vertices[] =
	{
		{ Vector3(-0.5f, 0.5f, -0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f), Vector3() },
		{ Vector3(0.5f, 0.5f, -0.5f), Vector4(0.0f, 1.0f, 0.0f, 1.0f), Vector3() },
		{ Vector3(0.5f, 0.5f, 0.5f), Vector4(0.0f, 1.0f, 1.0f, 1.0f), Vector3() },
		{ Vector3(-0.5f, 0.5f, 0.5f), Vector4(1.0f, 0.0f, 0.0f, 1.0f), Vector3() },
		{ Vector3(-0.5f, -0.5f, -0.5f), Vector4(1.0f, 0.0f, 1.0f, 1.0f), Vector3() },
		{ Vector3(0.5f, -0.5f, -0.5f), Vector4(1.0f, 1.0f, 0.0f, 1.0f), Vector3() },
		{ Vector3(0.5f, -0.5f, 0.5f), Vector4(1.0f, 1.0f, 1.0f, 1.0f), Vector3() },
		{ Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 0.0f, 1.0f), Vector3() },
	};
	for (int i = 0; i < 8; i++)
	{
		CubeVertex& vtx = vertices[i];
		vtx.Nor = vtx.Pos;    
		vtx.Nor.Normalize();  
	}

	D3D11_BUFFER_DESC bd = {};		   // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
	bd.Usage = D3D11_USAGE_IMMUTABLE;  // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage 
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

	// HW2.4) Register hard coding variables to Constant Buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(LightCBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pLightCBuffer);
	if (FAILED(hr))
		return hr;

	// HW2.4) Register hard coding variables to Constant Buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MtCBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pMtCBuffer);
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

	// HW2.4) Register hard coding variables to Constant Buffers
	LightCBuffer cbLight;
	cbLight.posLightCS = Vector3::Transform(Vector3(0, 8, 0), g_mView);
	// HW2.5) Add an option to the scene property : Paralle lighting or Point lighting
	if (g_lightFlag == 2) cbLight.lightFlag = 2;
	else cbLight.lightFlag = 1;
	cbLight.lightColor = Vector3(1.0f, 1.0f, 1.0f);
	g_pImmediateContext->UpdateSubresource(g_pLightCBuffer, 0, nullptr, &cbLight, 0, 0);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pLightCBuffer);

	// HW2.4) Register hard coding variables to Constant Buffers
	MtCBuffer mtb;
	mtb.mtcAmbient = Vector3(0.1f, 0.1f, 0.1f);
	mtb.shine = 10.f;
	mtb.mtcDiffuse = Vector3(0.7f, 0.7f, 0.f);
	mtb.mtcSpec = Vector3(0.2f, 0.f, 0.2f);
	g_pImmediateContext->UpdateSubresource(g_pMtCBuffer, 0, nullptr, &mtb, 0, 0);
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pMtCBuffer);

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

	//HW2.4) Register hard coding variables to Constant Buffers
	if (g_pTransformCBuffer) g_pTransformCBuffer->Release();
	if (g_pLightCBuffer) g_pLightCBuffer->Release();
	if (g_pMtCBuffer) g_pMtCBuffer->Release();
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
