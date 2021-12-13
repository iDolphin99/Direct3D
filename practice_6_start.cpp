// practice_1.cpp : Defines the entry point for the application.
// shading, lighting을 일반적인 3D model에 대해서 적용하는 것을 시도해보자 
// cube와 sphere에 대해서 modeling 했음 
// parametric formular로부터 위도와 경도를 계산하고 그것으로부터 계산된 Vertex position 그리고 이 position을 잇는 삼각형으로 이루어진 구, 
// 구는 위도와 경도 정보를 갖고 있는 texture coord 를 갖는 sphere가 된다 
// 이제 이 model을 한 scene에 여러개를 두어서 rendering하는 것 까지 template code다 
// 새로운 3D model을 읽어드려서 Rendering해보자 
// 3D model을 저장하는 여러가지 표준 format중 가장 직관적이고 많이 쓰는 format -> stl 
// stl format? STereoLithography
// stl은 binary로 저장될 수 있고, text 파일로도 저장될 수 있음 -> 우리는 binary, 용량이 클 경우 string으로 저장되어 있기 때문에 text 파일임 
// text 파일은 하나의 model에 대해서 facet(하나의 triangle)에 대해 noraml vector가 일일히 정의되어 있다 -> 수많은 facet이 적혀져 있는 것 

// 3d model을 읽어오기 전에 주의할 점 
// vertex position 은 3개가 있지만 normal은 face단위로 있기 때문에 vertex 3개가 같은 normal을 갖게 해서 rendering 해도 됨 
// 하지만 그 경우에는 noraml 값이 vertex에 동일한 attribute가 들어가면 memory 낭비
// face 단위로, triangle 단위로 저장되게 한 후 이 buffer를 shader에서 읽어들일 수 있게 하면 좋음 
// 물론 vertex의 attribute로 들어가게 되면 g_p에서 Input-Assembler stage-> Vertex shader로 streaming할 때 자동으로 normal 값을 읽어들일 수 있는 장점이 있긴 함, 하지만 GPU memory 낭비 발생 
// 물론 vertex 단위로 normal이 다르면 Vertex attribute vector로 들어가는 것이 좋다!

// sphere model을 만드는 것을 보면 u, v값이 펼쳐진 2D texture에 mapping되는 값임 (지구본 생각했을때) 
// texture mapping application -> environment map
// environment map을 cube를 통해서 구현하고 model에 reflection, refraction을 적용하는 실습을 하겠음

// 지난 실습에서 코딩한 내용은 Phonglighting까지 되고 있으며 
// Vertex의 attribute로 Cube에 Normal을 넣고 있었지만 이제는 Face단위로 Normal을 정의해서 shader resource로 PS에 넣고
// PS에서 System value로 (Semantics) PrimitiveID를 읽어올 수 있기 때문에 해당 value로부터 Normal Buffer를 Indexing해서 그 Normal Buffer를 가지고 Lighting 한 것 
// 즉, 기존에는 4개의 Vertex buffer가 각자 대각선 방향의 Normal이 있었음 
// 이제는 한 Face에 두 개의 삼각형이 있는데 두 삼각형이 같은 방향의 Normal (+y, -y...)을 가지게 됨 
// 그리고 이 Normal에 대해서 Lighting을 하기 때문에 예전에 Lighting 한 것 과는 다르게 우리가 알고 있는 친숙한 Lighting이 가능하다 
// STL model -> Standard format을 read/write 하는 function은 google, github을 통해서 얻을 수 있음 
// PrimitiveID를 통해 Normal을 가져오는.. 과정에서 시행착오를 거쳤죠..

#include "framework.h"
#include "Practice.h"
#include "stl_reader.h"
#include "stb_image.h"

#include <windowsx.h>
#include <stdio.h>

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>

#include <directxmath.h>
#include "SimpleMath.h"
#include <vector>
#include <map>
#include <string>

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

// 나중에 map 으로 처리하는 방법을 구현해보세요! 
ID3D11VertexShader* g_pVertexShaderPCN = nullptr;
ID3D11VertexShader* g_pVertexShaderPNT = nullptr;
ID3D11VertexShader* g_pVertexShaderP = nullptr;
ID3D11PixelShader* g_pPixelShader1 = nullptr;
ID3D11PixelShader* g_pPixelShader2 = nullptr;
ID3D11InputLayout* g_pIALayoutPCN = nullptr;
ID3D11InputLayout* g_pIALayoutPNT = nullptr;
ID3D11InputLayout* g_pIALayoutP = nullptr;
ID3D11Buffer* g_pVertexBuffer_cube = nullptr;
ID3D11Buffer* g_pNormalBuffer_cube = nullptr;
ID3D11ShaderResourceView* g_pSRV_cube = nullptr;
ID3D11Buffer* g_pVertexBuffer_stl = nullptr;
ID3D11Buffer* g_pNormalBuffer_stl = nullptr;
ID3D11ShaderResourceView* g_pSRV_stl = nullptr;
ID3D11Buffer* g_pVertexBuffer_sphere = nullptr;
ID3D11Buffer* g_pIndexBuffer_cube = nullptr;
ID3D11Buffer* g_pIndexBuffer_stl = nullptr;
ID3D11Buffer* g_pIndexBuffer_sphere = nullptr;
ID3D11Buffer* g_pCB_TransformWorld = nullptr;

ID3D11Texture2D* g_texEnvMap = nullptr;
ID3D11ShaderResourceView* g_tSRVEnvMap = nullptr;

ID3D11Texture2D* g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;

ID3D11RasterizerState* g_pRSState = nullptr;
ID3D11DepthStencilState* g_pDSState = nullptr;

ID3D11Buffer* g_pCB_Lights = nullptr;
Vector3 g_pos_light;
Vector3 g_pos_eye, g_pos_at, g_vec_up;

struct MyObject {
	// 여러 개의 object를 적용하는 코드 
	// 지금까지 작성한 code에 이어서 rendering function을 호출하면 됨 
	// 새롭게 생성한 모델을 rendering 다음에, rendering 다 한 다음에, 추가적으로 생성한 model을 GPU pl에 setting -> rendering 
	// 이런식으로 반복함으로써 여러개의 object를 rendering 할 수 있음 
	// 하지만 이런 방식은 구조를 복잡하게 만드므로 
	// 1) scene에 공동으로 들어가는 parameter를 camera state / light source / view port(Init device에서 setting)로 정리
	// camera state와 light source는 Constant buffer로 Scene parameter를 설정하는 Constant buffer로 설정
	// 2) 나머지 resource들에 대해서 
	// 각각의 object의 geometry를 저장하는 buffer, index를 저장하는 buffer
	// object가 OS -> WS로 배치되게 하는 world matrix를 저장하는 Material properties를 저장하는 Constant buffer를 object별로 지정 
	// object가 삼각형으로 그려지는지, index buffer의 사용 유무, 해당 vertex buffer가 어떤 구조로, attribute를 가지는지(input assembly layer - Noraml, texture...)
	// 해당 object를 VS 와 PS에서 그려주는 shader를 obejct의 parameter로 둠
	// object를 그리기 위한 rasterizer / depth buffer setting 을 어떻게 할 것인지 저장하는 parameter를, resource를 object별로 저장
	// 이보다 최적화된 코드를 작성할 수 있음 
	// MyObject의 object 단위로 여러 parameter들, GPU에 할당할 resource parameter를 저장할 variable들을 저장함 
public:
	// resources //
	ID3D11Buffer* pVBuffer;
	ID3D11Buffer* pIBuffer;
	ID3D11ShaderResourceView* pSRViewNormal; // noraml buffer를 가르키는 veiw를 통해서 indexing을 하기 때문에 이거을 MyObject structure의 memeber variable로 할당되어야 한다 
	ID3D11InputLayout* pIALayer;
	ID3D11VertexShader* pVShader;
	ID3D11PixelShader* pPShader;
	ID3D11RasterizerState* pRSState;
	ID3D11DepthStencilState* pDSState;

	UINT vb_stride;
	UINT ib_stride;
	int drawCount;
	Matrix mModel;
	Color mtAmbient, mtDiffuse, mtSpec;
	float shininess;

	MyObject() {
		ZeroMemory(this, sizeof(MyObject));
	}

	// 생성자에서 여기에 들어갈 필수정보들을 입력받게 해두었음 
	// 생성자의 input parameter에 대해서 그대로 struct의 parameter에 할당하게 했음 
	// struct 안에서 index, vertex buffer, VS, PS 이런 것들을 생성하게 만들 수 있음 
	// 그러나 그런 부분들은 지난 시간동안 만들었던 resource를 그대로 할당받아 사용할 수 있게끔 만들었음 
	MyObject(
		const ID3D11Buffer* pVBuffer_, const ID3D11Buffer* pIBuffer_, const ID3D11ShaderResourceView* pSRViewNormal_, // 구조체에 parameter를 입려갛는 사용자 입장에서는 const는 input parameter이고, 해당 함수 내에서 변하지 않는 parameter임을 표현함 
		const ID3D11InputLayout* pIALayer_, const ID3D11VertexShader* pVShader_, const ID3D11PixelShader* pPShader_,
		const ID3D11RasterizerState* pRSState_, const ID3D11DepthStencilState* pDSState_,
		const UINT vb_stride_, const UINT ib_stride_, const int drawCount_, const Matrix &mModel_,
		const Color& mtAmbient_, const Color& mtDiffuse_, const Color& mtSpec_, const float shininess_) {
		pVBuffer = (ID3D11Buffer*)pVBuffer_;
		pIBuffer = (ID3D11Buffer*)pIBuffer_;
		pIALayer = (ID3D11InputLayout*)pIALayer_;
		pVShader = (ID3D11VertexShader*)pVShader_;
		pPShader = (ID3D11PixelShader*)pPShader_;
		pRSState = (ID3D11RasterizerState*)pRSState_;
		pDSState = (ID3D11DepthStencilState*)pDSState_;
		pSRViewNormal = (ID3D11ShaderResourceView*)pSRViewNormal_; // const 썼기 때문에 casting 해줘야 함 
		vb_stride = vb_stride_;
		ib_stride = ib_stride_;
		drawCount = drawCount_;
		mModel = mModel_;
		mtAmbient = mtAmbient_;
		mtDiffuse = mtDiffuse_;
		mtSpec = mtSpec_;
		shininess = shininess_;
		pConstBuffer = nullptr;

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Matrix) + 4 * 4; // MUST BE times of 16
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		// 이 struct 구조 안에서 다음과 같이 obejct 별 Constant buffer(Wmatrix, material properties 저장하는)를 생성하게 해두었음  
		HRESULT hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &pConstBuffer); 
		if (FAILED(hr))
			MessageBoxA(NULL, "Constant Buffer of A MODEL ERROR", "ERROR", MB_OK);
	}

	// 해당 struct가 아래의 function이 호출 되면 
	// 지금 할당되어 있는 object 정보들을 가지고 Constant buffer를 update하게 했음 
	void UpdateConstanceBuffer() {
		struct ModelConst {
			Matrix mModel;
			DirectX::PackedVector::XMUBYTEN4 mtAmbient, mtDiffuse, mtSpec; // 4 4 4 
			float shininess; // 4 => 16 
		};
		// 현재 setting 되어있는 world matrix, material properties를 다음과 같이 sttructure에 넣고 이 structure를 가지고 해당 constantbuffer를 update해줌 
		ModelConst data = { mModel.Transpose() , mtAmbient.RGBA() , mtDiffuse.RGBA() , mtSpec.RGBA() , shininess };
		g_pImmediateContext->UpdateSubresource(pConstBuffer, 0, nullptr, &data, 0, 0);
	}

	// Getter로 해당 Constant buffer를 얻어오게 함 
	// get한 constant buffer를 가지고 Device context가 VS 또는 PS에 setting 하게 함 
	ID3D11Buffer* GetConstantBuffer() {
		return pConstBuffer;
	}

	// 이 구조차 안에서 buffer를 생성했기 때문에 구조체가 소멸할 때, 소멸하기 전에 반드시 delete function이 호출되게 해야 함 -> Release() 
	void Delete() {
		if (pConstBuffer) pConstBuffer->Release();
	}

private:
	// local resource //
	// 추가로 struct 안에서 constant buffer를 생성하게 했음 
	ID3D11Buffer* pConstBuffer; // transform, material colors...
};

// map standard library 
// what is map? -> dictionary, index에 대해서 value가 들어감
// std::string -> index, key // Myobject -> value 
// MyObject를 구별하는 식별자로 string을 사용할 것임 
std::map<std::string, MyObject> g_sceneObjs;

// http://www.songho.ca/opengl/gl_sphere.html
void GenerateSphere(const float radius, const int sectorCount, const int stackCount,
	std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords)
{
	// clear memory of prev arrays
	std::vector<float>().swap(positions);
	std::vector<float>().swap(normals);
	std::vector<float>().swap(texCoords);

	float x, y, z, xy;                              // vertex position
	float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
	float s, t;                                     // vertex texCoord

	float sectorStep = 2 * DirectX::XM_PI / (float)sectorCount;
	float stackStep = DirectX::XM_PI / (float)stackCount;
	float sectorAngle, stackAngle;

	for (int i = 0; i <= stackCount; ++i)
	{
		stackAngle = DirectX::XM_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			positions.push_back(x);
			positions.push_back(y);
			positions.push_back(z);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			normals.push_back(nx);
			normals.push_back(ny);
			normals.push_back(nz);

			// vertex tex coord (s, t) range between [0, 1]
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			texCoords.push_back(s);
			texCoords.push_back(t);
		}
	}
}

void GenerateIndicesSphere(const int sectorCount, const int stackCount,
	std::vector<UINT>& indices)
{
	// generate CCW index list of sphere triangles
	// k1--k1+1
	// |  / |
	// | /  |
	// k2--k2+1
	//std::vector<int> lineIndices;
	int k1, k2;
	for (int i = 0; i < stackCount; ++i)
	{
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}

			// store indices for lines
			// vertical lines for all stacks, k1 => k2
			//lineIndices.push_back(k1);
			//lineIndices.push_back(k2);
			//if (i != 0)  // horizontal lines except 1st stack, k1 => k+1
			//{
			//	lineIndices.push_back(k1);
			//	lineIndices.push_back(k1 + 1);
			//}
		}
	}
}

HRESULT Recompile(bool generateIALayout);

struct CB_TransformScene
{
	Matrix mView;
	Matrix mProjection;
};

struct CB_Object
{
	Matrix mWorld;
	DirectX::PackedVector::XMUBYTEN4 mtAmbient;
	DirectX::PackedVector::XMUBYTEN4 mtDiffuse;
	DirectX::PackedVector::XMUBYTEN4 mtSpec;
	float shininess;
};

struct CB_Lights
{
	Vector3 posLight;							 // light position 
	DirectX::PackedVector::XMUBYTEN4 lightColor; // int, light color는 int형 4byte로 두고 각각의 byte에 RGBA값을 넣음 

	Vector3 dirLight;							 // directional light의 direction을 저장하는 vector 
	int lightFlag;								 // light을 point? directional? 쓸지 shader에서 판정하기 위한 flag
};

struct CubeVertex
{
	Vector3 Pos;
	Vector4 Color;
	Vector3 Nor;
};
struct SphereVertex
{
	Vector3 Pos;
	Vector3 Nor;
	Vector2 Tex;
};

Matrix g_mWorld_cube, g_mWorld_sphere, g_mView, g_mProjection;

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
	static Vector3 pos_start_cam_ws, pos_start_at_ws, vec_start_up;
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
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_BACK: if (FAILED(Recompile(false))) printf("FAILED!!!!\n"); break;
		}
	}
	break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		pos_start_np_ws = ComputePosSS2WS(xPos, yPos, g_mView);
		pos_start_cam_ws = g_pos_eye;
		pos_start_at_ws = g_pos_at;
		vec_start_up = g_vec_up;
		mView_start = g_mView;
	}
	break;
	case WM_MOUSEMOVE:
		// WndProc mouse move
		// https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		// https://docs.microsoft.com/en-us/windows/win32/learnwin32/mouse-clicks
		if (wParam & MK_LBUTTON)
		{
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);
#pragma region HW part 1
			//printf("%f, %f, %f\n", pos_start_np_ws.x, pos_start_np_ws.y, pos_start_np_ws.z);
			Vector3 vec_start_cam2np = pos_start_np_ws - pos_start_cam_ws;
			vec_start_cam2np.Normalize();
			Vector3 vec_cur_cam2np = pos_cur_np_ws - pos_start_cam_ws;
			vec_cur_cam2np.Normalize();
			float angle_rad = acosf(vec_start_cam2np.Dot(vec_cur_cam2np)) * 3.0f;
			Vector3 rot_axis = vec_start_cam2np.Cross(vec_cur_cam2np);
			if (rot_axis.LengthSquared() > 0.000001)
			{
				printf("%f\n", angle_rad);
				rot_axis.Normalize();
				Matrix matR = Matrix::CreateFromAxisAngle(rot_axis, angle_rad);

				g_pos_eye = Vector3::Transform(pos_start_cam_ws, matR);
				//g_pos_at = no change
				g_vec_up = Vector3::TransformNormal(vec_start_up, matR);
			}
#pragma endregion HW part 1
			g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);
		}
		else if (wParam & MK_RBUTTON)
		{
			Vector3 pos_cur_np_ws = ComputePosSS2WS(xPos, yPos, mView_start);
#pragma region HW part 2
			float dist_at = (pos_start_at_ws - pos_start_cam_ws).Length();
			// np : 0.01f
			Vector3 vec_diff_np = pos_cur_np_ws - pos_start_np_ws;
			float dist_diff_np = vec_diff_np.Length();
			float dist_diff = dist_diff_np / 0.01f * dist_at;
			if (dist_diff_np > 0.000001)
			{
				Vector3 vec_diff = vec_diff_np / dist_diff_np * dist_diff;
				g_pos_eye = pos_start_cam_ws + vec_diff;
				g_pos_at = pos_start_at_ws + vec_diff;
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

HRESULT Recompile(bool generateIALayout)
{
	HRESULT hr = S_OK;
	// Compile the vertex shader
	ID3DBlob *pVSBlobPCN = nullptr, *pVSBlobPNT = nullptr, *pVSBlobP = nullptr;
	
	auto CreateShader = [](const std::string& shaderName, const std::string& shaderProfile, ID3DBlob **ppShaderBlob,
		ID3D11DeviceChild **ppShader)
	{
		if (shaderProfile != "vs_4_0" && shaderProfile != "ps_4_0") return E_FAIL;

		HRESULT hr = CompileShaderFromFile(L"Shaders_6_start.hlsl", shaderName.c_str(), shaderProfile.c_str(), ppShaderBlob);
		if (FAILED(hr))
		{
			MessageBoxA(nullptr, (std::string("Shader Compiler Error!! ") + shaderName + " " + shaderProfile).c_str(), "Error!!", MB_OK);
			return hr;
		}

		if (shaderProfile == "vs_4_0") {
			ID3D11VertexShader **ppVS = (ID3D11VertexShader**)ppShader;
			if (*ppVS) (*ppVS)->Release();
			// Create the vertex shader
			hr = g_pd3dDevice->CreateVertexShader((*ppShaderBlob)->GetBufferPointer(), (*ppShaderBlob)->GetBufferSize(), nullptr, ppVS);
		}
		else if (shaderProfile == "ps_4_0") {
			ID3D11PixelShader** ppPS = (ID3D11PixelShader**)ppShader;
			if (*ppPS) (*ppPS)->Release();
			// Create the vertex shader
			hr = g_pd3dDevice->CreatePixelShader((*ppShaderBlob)->GetBufferPointer(), (*ppShaderBlob)->GetBufferSize(), nullptr, ppPS);
		}

		return hr;
	};

	hr |= CreateShader("VS_PCN", "vs_4_0", &pVSBlobPCN, (ID3D11DeviceChild**)&g_pVertexShaderPCN);
	hr |= CreateShader("VS_PNT", "vs_4_0", &pVSBlobPNT, (ID3D11DeviceChild**)&g_pVertexShaderPNT);
	hr |= CreateShader("VS_P", "vs_4_0", &pVSBlobP, (ID3D11DeviceChild**)&g_pVertexShaderP);

	if (generateIALayout && SUCCEEDED(hr)) {
		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout_PCN[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout_PCN);
		// Create the input layout
		hr |= g_pd3dDevice->CreateInputLayout(layout_PCN, numElements, pVSBlobPCN->GetBufferPointer(), pVSBlobPCN->GetBufferSize(), &g_pIALayoutPCN);

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout_PNT[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		numElements = ARRAYSIZE(layout_PNT);
		// Create the input layout
		hr |= g_pd3dDevice->CreateInputLayout(layout_PNT, numElements, pVSBlobPNT->GetBufferPointer(), pVSBlobPNT->GetBufferSize(), &g_pIALayoutPNT);

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout_P[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		numElements = ARRAYSIZE(layout_P);
		// Create the input layout
		hr |= g_pd3dDevice->CreateInputLayout(layout_P, numElements, pVSBlobP->GetBufferPointer(), pVSBlobP->GetBufferSize(), &g_pIALayoutP);
	}

	if (pVSBlobPCN) pVSBlobPCN->Release();
	if (pVSBlobPNT) pVSBlobPNT->Release();
	if (pVSBlobP) pVSBlobP->Release();     // reference count가 counting 되는 resource에 대해서는 다 쓰고 나면 무조건 Release! 
	
	ID3DBlob *pPSBlob1 = nullptr, *pPSBlob2 = nullptr;
	hr |= CreateShader("PS1", "ps_4_0", &pPSBlob1, (ID3D11DeviceChild**)&g_pPixelShader1);
	hr |= CreateShader("PS2", "ps_4_0", &pPSBlob2, (ID3D11DeviceChild**)&g_pPixelShader2);
	if (pPSBlob1) pPSBlob1->Release();
	if (pPSBlob2) pPSBlob2->Release();

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
#ifdef MSAA
	sd.SampleDesc.Count = 2;
#else
	sd.SampleDesc.Count = 1;
#endif
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
#ifdef MSAA
	descDepth.SampleDesc.Count = 2;
#else
	descDepth.SampleDesc.Count = 1;
#endif
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
#ifdef MSAA
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
#else
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
#endif
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
	Recompile(true);
#pragma endregion Create Shader


	int indices_cube = 0, indices_stl = 0, indices_sphere = 0;
#pragma region Create a cube
	{
		// Cube -> position, color, normal 정보가 들어감 
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
		// normal 값을 채우는 logic 
		// shader code -> semantic : application level에서 어떤 attirbute 정보를 줄 수 있고, shader 내부에서 g_p 통해서 어떤 정보를 얻을 수 있는지에 대해 명시한 string -> semantics 
		// 별도로 만든 noraml buffer에 대해, face에 순서대로 들어가게 해야함 
		// noraml 정보를 array로 만들어서 array index가 2인 noraml은 ~face에 해당하는 noraml로 정의할 수 있다 
		// 예를 들어서 VS에서 세 개의 점들이 들어오는데 이 점이 어떤 face에서 들어왔는지를 알 수 있는 정보가 필요함 
		// 그리고 이에 대해서 semantics에서는 해당 정보를 주는 것이 있음 -> SV_PrimitiveID 

		for (int i = 0; i < 8; i++)
		{
			CubeVertex& vtx = vertices[i];
			vtx.Nor = vtx.Pos;
			vtx.Nor.Normalize();
		}

		D3D11_BUFFER_DESC bd = {}; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_buffer_desc
		bd.Usage = D3D11_USAGE_IMMUTABLE; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage 
		bd.ByteWidth = sizeof(CubeVertex) * ARRAYSIZE(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_cube);
		if (FAILED(hr))
			return hr;

		// Create index buffer
		// 8개의 vertex의 index를 가지고 triangle을 그림으로써 최종적으로 cube를 그림 
		// Vertex buffer의 index값 
		WORD indices[] =
		{
			3,1,0,  // 두 개의 삼각형 -> 하나의 face 
			2,1,3,  // +y

			0,5,4,
			1,5,0,  // -z

			3,4,7,
			0,4,3,  // -x

			1,6,5,
			2,6,1,  // +x

			2,7,6,
			3,7,2,  // +Z

			6,4,5,
			7,4,6,  // -y
		};
		indices_cube = ARRAYSIZE(indices);

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(WORD) * indices_cube;        // 36 vertices needed for 12 triangles in a triangle list
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = indices;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer_cube);
		if (FAILED(hr))
			return hr;

		// +y, -z, -x, +x, +z, -y
		// face 단위 Normal vector 만들기 
		// bd (description) -> ByteWidth, BindFlag 수정 
		// InitData 수정해서 buffer 생성 
		// 해당 Normal 정보는 OS에서 정의 
		Vector3 cubeFaceNormlas[] = {
			// 두 개의 삼각형이 하나의 normal을 공유하기 때문에 두 삼각형의 normal을 맞춰주는 방식으로 코딩합니다 
			Vector3(0, 1.f, 0), Vector3(0, 1.0f, 0),
			Vector3(0, 0, -1.f), Vector3(0, 0, -1.f),
			Vector3(-1.f, 0, 0), Vector3(-1.f, 0, 0),
			Vector3(1.f, 0, 0), Vector3(1.f, 0, 0),
			Vector3(0, 0, 1.f), Vector3(0, 0, 1.f),
			Vector3(0, -1.f, 0), Vector3(0, -1.f, 0),
		};

		bd.ByteWidth = sizeof(Vector3) * 6;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		InitData.pSysMem = cubeFaceNormlas;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pNormalBuffer_cube);
		if (FAILED(hr))
			return hr;

		// 중요한 것은 다른 buffer resource와 다르게 shader resource들은 shader에 해당 resource를 set 하기 위해서 View라는 interface를 사용함 (외워) 
		// 해당 shader resource를 바로 shader에 set이 아닌 shader를 가르키는 view라는 interface를 먼저 만들어야 함 -> syntax 
		D3D11_SHADER_RESOURCE_VIEW_DESC dcSrv = {};
		dcSrv.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT; // vector3 -> directx format 에서는 r32f32b32 -> 32bit 3channel float으로 format 작성 
		dcSrv.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;		 // view가 가르키는 resource가 어떤 resource다를 가르킴 -> 이 shader resource view description이 비단 buffer뿐만 아니라 제 3의 resource들을 가르키는 view일수도 있고 해당 resource에 대해서 서로 다른 desc를 갖게 되는데 이를 BUFFEREX라고 함, BUFFER EXTENSION DESC  
		dcSrv.BufferEx.FirstElement = 0;
		dcSrv.BufferEx.NumElements = 6;
		hr = g_pd3dDevice->CreateShaderResourceView(g_pNormalBuffer_cube, &dcSrv, &g_pSRV_cube);
		if (FAILED(hr))
			return hr;
		// 이렇게 생성된 shader resource view는 normal buffer를 directly set하지 않음 
		// noraml buffer를 가르키는 veiw를 통해서 indexing을 하기 때문에 이거을 MyObject structure의 memeber variable로 할당되어야 한다 
	}
#pragma endregion

#pragma region Load a STL
	{
		// 값을 읽어오는 interface, 별도로 interface를 통해서 별도의 struct에 copy하는 것이 아니라 저장되어 있는 struct를 directly하게 접근하는 방법을 사용하겠음 
		// 우리는 vector3를 쓰고 있는데 vector3는 float이 3개짜리 array랑 똑같이 memory에 저장되어 있음 
		// STL은 vertex의 position 정보만 넣는 것으로 하겠음 
		// 즉, input assembly에 vertex buffer만 들어가게 함, index buffer는 쓰지 않겠음 
		// 즉, STL용 vertex shader를 별도로 만들겠다는 의미 

		// memory에 할당되어 있는 GPU를 넘겨주기 위해서이기 때문에.. 
		stl_reader::StlMesh <float, unsigned int> mesh("Armadillo2.stl");
		const float* raw_coords = mesh.raw_coords();    // element들을 나타냄 
		// Camera position을 정의해야 함, Armadildo의 bounding box를 확인해야 함 
		/*const Vector3* raw3_coords = (const Vector3 *)raw_coords;
		Vector3 minPos(FLT_MAX), maxPos(-FLT_MAX);
		for (int i = 0; i < mesh.num_vrts(); i++) {
			minPos.x = min(minPos.x, raw_coords[i].x);
			minPos.x = min(minPos.x, raw_coords[i].y);
			minPos.x = min(minPos.x, raw_coords[i].z);

			minPos.x = max(minPos.x, raw_coords[i].x);
			minPos.x = max(minPos.x, raw_coords[i].y);
			minPos.x = max(minPos.x, raw_coords[i].z);
		}
		*/
		const float* raw_normals = mesh.raw_normals();
		const unsigned int* raw_tris = mesh.raw_tris(); // 삼각형의 vertex의 index를 저장함

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(Vector3) * mesh.num_vrts();
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = raw_coords;											// vector3로 들어가도 되지만 memory size상으로는 같기 때문에 meomory copy가 일어나게 되면 저장된 것을 float3로 가져다 써도 됨 
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer_stl);
		if (FAILED(hr))
			return hr;

		indices_stl = mesh.num_tris() * 3;
		// Index Buffer 
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(UINT) * indices_stl;        // 36 vertices needed for 12 triangles in a triangle list
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = raw_tris;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer_stl);
		if (FAILED(hr))
			return hr;

		bd.ByteWidth = sizeof(Vector3) * mesh.num_tris();
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		InitData.pSysMem = raw_normals;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pNormalBuffer_stl);
		if (FAILED(hr))
			return hr;

		D3D11_SHADER_RESOURCE_VIEW_DESC dcSrv = {};
		dcSrv.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		dcSrv.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		dcSrv.BufferEx.FirstElement = 0;
		dcSrv.BufferEx.NumElements = mesh.num_tris();
		hr = g_pd3dDevice->CreateShaderResourceView(g_pNormalBuffer_stl, &dcSrv, &g_pSRV_stl);
		if (FAILED(hr))
			return hr;
	}
#pragma endregion


#pragma region Create a sphere
	{
		// 위도와 경도에 해당하는 stacks와 sectors로 spher를 parameter화 해서 이것으로부터 3차원 model을 생성해주는 code 
		// OpenGL Sphere

		// Vertex 생성, vertex 마다 position, noraml, texture coordinate을 가짐
		// texture coordinate는 sphere object가 flatten 되었을 때 사각형 map이 가로 세로가 각각 0~1로 mapping된다 했을 때 
		// 각 vertex point에서 해당 texture에 mapping되는 coordinate을 저장함
		// vector : 동적으로 추가할 수 있는 array, <float> : 기본 타입이 float, vertex와 noraml은 float 3개로 되는 것임 
		// 만약 vertex가 10개라면 vector array안에 들어가는 element들은 30개가 됨 
		// 그러나 texcoord는 u,v 값, 2차원 값이기 때문에 10개의 vertex에 대해서 20개의 element를 갖게 됨
		// stackcount, sectorcount -> 얼마나 정밀하게 stack, sector를 설정할지에 대한 parameter 
		// 이 parameter에 대해서 sphere를 위한 vertex buffer, index buffer를 생성하는 코드를 추가함 
		std::vector<float> position, normal, texcoord;
		const int stackCount = 100, sectorCount = 100;

		GenerateSphere(0.5f, sectorCount, stackCount, position, normal, texcoord); // 반지름이 0.5f인 구 
		std::cout << "# of position: " << position.size() / 3 << ", # of normal: " << normal.size() / 3 << ", # of texcoord: "
			<< texcoord.size() / 2 << std::endl;

		int numVertices = (int)position.size() / 3;

		D3D11_BUFFER_DESC bd_sphere = {};
		bd_sphere.Usage = D3D11_USAGE_IMMUTABLE;
		bd_sphere.ByteWidth = sizeof(SphereVertex) * numVertices;
		bd_sphere.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd_sphere.CPUAccessFlags = 0;

		std::vector<SphereVertex> verticesSphere(numVertices);
		for (int i = 0; i < numVertices; i++)
		{
			SphereVertex& vtx = verticesSphere[i];

#define VECTOR3std(VEC_NAME, I) Vector3(VEC_NAME[3 * I + 0], VEC_NAME[3 * I + 1], VEC_NAME[3 * I + 2])

			vtx.Pos = VECTOR3std(position, i);
			vtx.Nor = VECTOR3std(normal, i);
			vtx.Tex = Vector2(texcoord[2 * i + 0], texcoord[2 * i + 1]);
		}

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = &verticesSphere[0];
		hr = g_pd3dDevice->CreateBuffer(&bd_sphere, &InitData, &g_pVertexBuffer_sphere); // sphere를 위한 vertex buffer, index buffer를 생성하는 코드를 추가함
		if (FAILED(hr))
			return hr;

		// 삼각형을 그릴때는 indexing을 해서 그리기 때문에 index buffer를 설정하게 됨 
		// index buffer를 생성하는 code 
		// 만약 생성하는 삼각형이 10개이면 index buffer의 element의 개수는 30개가 됨 
		std::vector<UINT> sphereIndices;
		GenerateIndicesSphere(sectorCount, stackCount, sphereIndices);

		indices_sphere = sphereIndices.size();

		bd_sphere.Usage = D3D11_USAGE_DEFAULT;
		bd_sphere.ByteWidth = sizeof(int) * (UINT)indices_sphere;        // 36 vertices needed for 12 triangles in a triangle list
		bd_sphere.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd_sphere.CPUAccessFlags = 0;
		InitData.pSysMem = &sphereIndices[0];
		hr = g_pd3dDevice->CreateBuffer(&bd_sphere, &InitData, &g_pIndexBuffer_sphere); // sphere를 위한 vertex buffer, index buffer를 생성하는 코드를 추가함
		if (FAILED(hr))
			return hr;
	}
#pragma endregion Create a sphere

	// Create the constant buffer
	// scene에 공통으로 사용되는 transform matrix를 저장하는 constant buffer 
	// -> object 단위로 transform되는 world Matrix / scene단위로 (개별 camear단위로) 설정해야 하는 transform matrix : camera matrix, projection matrix
	// light resource 를 저장하는 constant buffer 
	D3D11_BUFFER_DESC bdCB = {};
	bdCB.Usage = D3D11_USAGE_DEFAULT;
	bdCB.ByteWidth = sizeof(CB_TransformScene);							   // sturcture를 저장할 수 있을 정도의 크기로 buffer를 생성 
	bdCB.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bdCB.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bdCB, nullptr, &g_pCB_TransformWorld); // 생성된 buffer는 rendering function에서 해당 Cbuffer를 update 함 
	if (FAILED(hr))
		return hr;

	bdCB.Usage = D3D11_USAGE_DEFAULT;
	bdCB.ByteWidth = sizeof(CB_Lights);
	bdCB.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bdCB.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bdCB, nullptr, &g_pCB_Lights);
	if (FAILED(hr))
		return hr;

#pragma region Env Mapping
	// texture 의 4 channel 짜리 값을 넣기 위해, alpha channel 까지 넣기 위해서 img_rgba를 4 channel 짜리로 만듬 
	// 그래서 alpha에 해당하는 값은 255값을 넣고 있음 
	// 그리고 항상 new를 사용하여 할당하면 logic이 끝날 때 내가 만든 것에 대해 delocation 해줘야 함
	// 안그러면 programming이 종료되어도 계속 memory가 잡혀 있기 때문 -> 이런게 쌓이다 보면 블루스크린 뜸, memory allocation이 중요한 이유 
	int w, h, n;
	unsigned char* img = stbi_load("envmap.jpg", &w, &h, &n, 0);
	unsigned char* img_rgba = new unsigned char[w * h * 4];
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			img_rgba[4 * (w * y + x) + 0] = img[3 * (w * y + x) + 0];
			img_rgba[4 * (w * y + x) + 1] = img[3 * (w * y + x) + 1];
			img_rgba[4 * (w * y + x) + 2] = img[3 * (w * y + x) + 2];
			img_rgba[4 * (w * y + x) + 3] = (unsigned char)255;
		}

	// D3D11 : structure, {} : structure 안에 들어가는 값을 모두 Null을 넣어줌 
	// ZeorMemory(&dcTex2D, sizeof(D3D11_TEXURE2D_DESC)) 이 texture size만큼에 대해서 해당 메모리가 지정하고 있는 위치에 zero 값을 넣는다. -> C style 
	// D3D11_TEXTURE2D_DESC dcTex2D struct 안에 들어가는 parameter는 무엇이 있는가? -> Doc 참조 
	// w, h : image의 size만큼 2D texture로 잡은 것 
	// Format : texture는 4byte 단위로 정의되어야 한다, channel은 3개짜리로 쓸 수 있음 왜냐하면 buffer이런거 잡을 때 format을 3차원짜리 Vector를 썼었음, x,y,z가 float이였음 모두, 12byte였고, 4의 배수니까 
	// 그런데 texture image는 각각의 channel이 1byte이기 때문에 RGB 1byte씩 잡으면 3vyte니까 잡을 수 없음, 4의 배수가 아니라서, 그래서 A8을 추가해서 4byte로 저장한건데 이를 위해서 위에서 A값을 255로 넣은 것임
	// 이렇게 하는 이유는 texture를 생성할 때 initialize 하는 resource를 넣어주기 때문임, 데이터를 초기화 안하고 나중에 resource를 mapping해서 개별적으로 setting할 수 있지만 일단 통으로 initialize해서 사용하는 방법을 알려드림 
	// ArraySize : Texture가 array로 지정될 수 있기 때문에, 지금은 array로 쓰지 않고 있음 
	// MipLevel : mipmap level로 쓸 수 있음 
	// SampleDesc.Count/Quality : Texture를 Sampling할 때 Super Sampling하느냐 에 대한 option, 그러나 보통은 Supersampling으로 해결되는 문제가 아니기 때문에 쓰지 않음, count -> texel마다 하나의 값을 저장하겠다 => 대부분의 경우 0,1로 두면 됨  
	// Usage 는 알아두세요! Default : GPU에서 read/write이 되는 것, GPU에서 resource를 읽어서 rendertargetbuffer에 그림을 그린다 -> GPU에서 모두 수행하는 것 => 따라서 render target buffer는 반드시 DEFAULT 
	// IMMUTABLE : GPU에서 read 만 되고 write은 안됨, model data가 있는데 더이상 수정이 안되는 static data라면 IMMUTABLE로 하면 좋다 -> 당연히 operation 제약이 많을 수록 memory access 효율이 좋아진다 
	// DYNAMIC : GPU에서는 read만, CPU에서는 write만 되는 것 -> constant buffer 용으로 많이 씀 -> UpdateSubresource 확인해보자! 
	// BINDflag : resource가 buffer type? 다른 type? -> 우리는 shader resource로 사용할 것임 
	// CPUAccessFlag : IMMUTABLE로 했기 때문에 CPU access가 안되는 resource임, NULL 
	// MiscFlag : Mipmap을 허용하냐? 안하냐? -> 허용 안함, null 
	D3D11_TEXTURE2D_DESC dcTex2D = {};
	dcTex2D.Width = w;
	dcTex2D.Height = h;
	dcTex2D.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	dcTex2D.ArraySize = 1;
	dcTex2D.MipLevels = 1;  
	dcTex2D.SampleDesc.Count = 1;
	dcTex2D.SampleDesc.Quality = 0;
	dcTex2D.Usage = D3D11_USAGE_IMMUTABLE;
	dcTex2D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dcTex2D.CPUAccessFlags = 0;
	dcTex2D.MiscFlags = 0;

	// 위의 discription을 가지고...
	// GPU에 resource를 만들 때는 Device를 사용함, context가 아니라
	// Device를 가지고 2D Texture를 만드는데 이때 만들 때 resource를 discription 하는 것은 위에 것을 쓰고, Initialization buffer는 위의 내용 
	// SysMemPitch : 사실 img_rgba는 1D buffer임, texture는 근데 2D임, 그래서 첫 번째 y=0인거에 대해서는 몇개, y=1일때는 몇개, stride 개수를 정의하는 정보, width만큼인데 4channel짜리니까 4개씩  
	// SysMemSlicePitch : 3차원 data일때 쓰는 것 
	// 이렇게 만들어진 2D texture를 &g_texEnvMap에 저장
	// 이렇게 Texture도 IMMUTABLE로 잘 만들어졌고, INITIALIZATION도 잘 되어있음 
	D3D11_SUBRESOURCE_DATA InitTex2DData;
	InitTex2DData.pSysMem = img_rgba;
	InitTex2DData.SysMemPitch = w * 4;
	InitTex2DData.SysMemSlicePitch = 0;
	hr = g_pd3dDevice->CreateTexture2D(&dcTex2D, &InitTex2DData, &g_texEnvMap);
	delete[] img_rgba;
	if (FAILED(hr)) {
		return hr;
	}

	// 2D texture를 shader에 setting 하기 위한 view resource 만들기 -> Normal buffer를 만들던 때와 비슷함 
	// Format : texture foramt과 똑같은 것을 사용 
	// ViewDimension : Normal Buffer일때는 Bufferex를 썼는데 이번에는 texture2d dimension을 사용 
	// Miplevle : 하나 짜리 쓰니까 1 , MostDetailedMip : Mipmap 쓰지 않으니까 0번째 level data가 original data => 이렇게 쓰면 된다 정도만 알아두세요 
	// 해당 resource가 g_tSRVEnvMap view에 mapping됨 
	// 이 view는 env map 정보이며 이 view를 GPU pipeline에 mapping할 것인데 이 view를 object단위의 parameter를 넣을지 아니면 scene단위의 parameter를 넣을지 고민해야 함 
	// 근데 이는 env map 자체이기 때문에 scene paramter로 두겠음, 마치 light을 scenen paramter로 둔 것 처럼 
	// 근데 원래는 object단위로 넣었는데 현장에서 바로 global로 넣겠음
	D3D11_SHADER_RESOURCE_VIEW_DESC dcTexSRV = {};
	dcTexSRV.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	dcTexSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; //55분쯤
	dcTexSRV.Texture2D.MipLevels = 1;
	dcTexSRV.Texture2D.MostDetailedMip = 0;
	hr = g_pd3dDevice->CreateShaderResourceView(g_texEnvMap, &dcTexSRV, &g_tSRVEnvMap);
	if (FAILED(hr)) {
		return hr;
	}

#pragma endregion 

#pragma region Transform Setting
	g_pos_light = Vector3(0, 8, 0);

	g_mWorld_cube = Matrix::CreateScale(10.f);
	// OS에서는 직경이 1인 sphere가 원점을 중심으로 modeling 되어 있음 -> 이 구를 직경이 2인 구로 두고 이 구를 point lihgt position에 둠  
	// DirectX math는 row major -> transform이 왼쪽에서 오른쪽으로 진행 -> scaling 먼저 하고(구가 원점을 중심으로 2배) translation을 함(light position에)
	g_mWorld_sphere = Matrix::CreateScale(2.f) * Matrix::CreateTranslation(g_pos_light); 

	g_pos_eye = Vector3(0.0f, 0.0f, 20.0f);
	g_pos_at = Vector3(0.0f, 0.0f, 0.0f);
	g_vec_up = Vector3(0.0f, 1.0f, 0.0f);
	g_mView = Matrix::CreateLookAt(g_pos_eye, g_pos_at, g_vec_up);

	g_mProjection = Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV2, width / (FLOAT)height, 0.01f, 1000.0f);
#pragma endregion

#pragma region States
	// rasterizer state를 하나 더 만들어서 object별로 다르게 setting 되도록 하였음 -> 실습에서는 시간관계로 backface culling을 쓰지 않음 
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
#ifdef MSAA
	descRaster.MultisampleEnable = true;
	descRaster.AntialiasedLineEnable = true;
#else
	descRaster.MultisampleEnable = false;
	descRaster.AntialiasedLineEnable = false;
#endif
	hr = g_pd3dDevice->CreateRasterizerState(&descRaster, &g_pRSState);

	D3D11_DEPTH_STENCIL_DESC descDepthStencil;
	ZeroMemory(&descDepthStencil, sizeof(D3D11_DEPTH_STENCIL_DESC));
	descDepthStencil.DepthEnable = true;
	descDepthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	descDepthStencil.StencilEnable = false;
	hr = g_pd3dDevice->CreateDepthStencilState(&descDepthStencil, &g_pDSState);
#pragma endregion

	// Sampler라는 resource를 만들고 이를 set해줘도 됨, 다른 Raster state와 마찬가지로 
	// 그러나 번거롭기 때문에 그냥 간단하게 hlsl에서 만들어도 됨 
	D3D11_SAMPLER_DESC dcSample = {};
	dcSample.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	dcSample.AddressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	dcSample.AddressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	dcSample.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	*(Vector4*)&dcSample.BorderColor = Vector4();
	dcSample.MipLODBias = 0;
	dcSample.MaxAnisotropy = 16;
	dcSample.MinLOD = 0;
	dcSample.MaxLOD = D3D11_FLOAT32_MAX;
	dcSample.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//hr = g_pd3dDevice->CreateSamplerState(&dcSample, &g_samplerTex2D);
	//if (FAILED(hr)) {
	//	return hr;
	//}




	// setting 된 것을 가지고 map을 전역변수로 만들었었음 
	// "CUBE"라는 이름으로 object에 들어가는 정보들을 setting함 -> GPU resource들은 전역변수로 빼서 관리하고 있기 때문에 MyObject에서 이를 삭제하거나 그러지 않음 
	// 그러나 밑에 처럼 한 이유는 g_pVertexBuffer_cube를 여러개 만들 수 있음 
	// 개별 object는(MyObject) object 단위의 constant buffer가 있고, structure가 생성됨과 동시에 생성자 안에서 constant buffer가 생성됨 
	// object를 rendering할 때 사용되는 W_transform, material properties를 저장하는 Constant buffer를 MyObject에서 생성되게 했음 
	g_sceneObjs["CUBE"] = MyObject(g_pVertexBuffer_cube, g_pIndexBuffer_cube, g_pSRV_cube, g_pIALayoutPCN, g_pVertexShaderPCN, g_pPixelShader1,
		g_pRSState, g_pDSState, sizeof(CubeVertex), sizeof(WORD), indices_cube,
		g_mWorld_cube, Color(0.1f, 0.1f, 0.1f), Color(0.7f, 0.7f, 0), Color(0.2f, 0, 0.2f), 10.f);

	// input assembly layout을 수정해야 함 -> vertex Buffer의 point 정보만 들어가고 있기 때문 
	g_sceneObjs["STL"] = MyObject(g_pVertexBuffer_stl, g_pIndexBuffer_stl, g_pSRV_stl, g_pIALayoutP, g_pVertexShaderP, g_pPixelShader1,
		g_pRSState, g_pDSState, sizeof(Vector3), sizeof(UINT), indices_stl,
		Matrix::CreateScale(1.f/12.f) * Matrix::CreateTranslation(-10.f, 0, 0), Color(0.1f, 0.1f, 0.1f), Color(0.7f, 0, 0.7f), Color(0.2f, 0, 0.2f), 10.f);

	// 가려져서 안그려져야 하는데 빛이 보이고 있음
	// 객체와 객체 사이의 geometry interaction은 수행하고 있지 않음
	// view 기준의 backface가 아니냐를 판정해서 specular term을 없애는 과제였음
	// 그렇기에 local lighting을 잘 보여주는 예시가 된다 
	// Diffuse term을 노란색으로 두지 않았는데 노란색으로 보임 -> shader code를 hard coding 했기 때문 -> object단위의 Constant buffe가 제대로 setting되어 있지 않음
	// 수정 후 초록색 빛이 보임 
	// 이는 G에 대한 빛을 쐈기 때문에 G,B 색을 가진 Cube여도 G빛만 반사하기 때문에 초록색으로 보임 
	// 즉, 물체의 Diffuse color가 R,G 라면 lightColor가 1,1,1 white를 보내도 R,G의 color(yellow)만 반사시킴! 
	g_sceneObjs["CUBE_2"] = MyObject(g_pVertexBuffer_cube, g_pIndexBuffer_cube, NULL,g_pIALayoutPCN, g_pVertexShaderPCN, g_pPixelShader1, // face normal vector가 없음 -> NULL 
		g_pRSState, g_pDSState, sizeof(CubeVertex), sizeof(WORD), indices_cube,
		Matrix::CreateScale(5.f) * Matrix::CreateTranslation(10.f, 0, 0), Color(0.1f, 0.1f, 0.1f), Color(0, 0.7f, 0.7f), Color(0.2f, 0, 0.2f), 10.f);

	g_sceneObjs["SPHERE"] = MyObject(g_pVertexBuffer_sphere, g_pIndexBuffer_sphere, NULL, g_pIALayoutPNT, g_pVertexShaderPNT, g_pPixelShader2,
		g_pRSState, g_pDSState, sizeof(SphereVertex), sizeof(UINT), indices_sphere,
		g_mWorld_sphere, Color(1.f, 1.f, 1.f), Color(), Color(), 1.f);

	// World_matrix에서 orientation을 주는 transform 을 안하고 Scaling만 주는 matrix를 사용하였기 때문에, orientation을 바꾸는 transform이 필요함 
	// x축으로 적당히 회전해도 되지만 우리는 look at function을 사용하자 
	// z 축 방향을 y축방향이 되게 해야 한다 -> CS에서 보는 방향의 -방향이 z축 방향이였음 -> lookat position이 들어감 
	// 첫 번째 element : CS의 원점은 WS의 원점과 같다, 두 번째 elelment : CS가 보는 방향이 구의 0, -1, 0 이여야 한다, 세 번째 element : upvector, y축인데 그대로 오면 됨 
	Matrix matR = Matrix::CreateLookAt(Vector3(0, 0, 0), Vector3(0, -1, 0), Vector3(0, 0, 1));

	// 직경이 500인 구가 center에 놓임 -> Backface cullling이 일어났기 때문에 보이지 않음 
	// 카메라가 구 안으로 들어가면 구 안에서 보면 구의 안쪽 면, Backface를 보게 되는 것이고 Backface에 대해서는 culling out이 되고 있기 때문임
	// 카메라 setting할 때 np, fp을 설정했을 때 가장 멀리 있는 것이 100이였음, 카메라의 위치가 20일때 가장 멀리있는 것을 보아도 -80에 있는 것을 볼 수 있음 
	// sphere는 더 멀리 있음, 그렇기 때문에 camera fp을 1000정도로 늘림 
	// 원점에서 Translation이 일어나지 않으니까 Scale, Rotation 순서는 상관없지만 통상적으로 Orientation먼저 잡고 가자
	g_sceneObjs["SPHERE_ENV"] = MyObject(g_pVertexBuffer_sphere, g_pIndexBuffer_sphere, NULL, g_pIALayoutPNT, g_pVertexShaderPNT, g_pPixelShader2,
		g_pRSState, g_pDSState, sizeof(SphereVertex), sizeof(UINT), indices_sphere,
		matR * Matrix::CreateScale(20.f), Color(1.f, 1.f, 1.f), Color(), Color(), 1.f);

	return hr;
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
#pragma region Common Scene (World)
	// view, projection transform은 VS에서만 쓰일 것이기 때문에 VS에만 setting 
	// light source는 goraud shading을 사용하지 않기 때문에(phong shading을 쓰기 때문에) VS에는 setting할 필요 없음, PS에만 setting -> 최적화하기 위해서 
	// scene에 공통으로 적용될 Constant buffer를 다음코드들에서 setting 함 
	CB_TransformScene cbTransformScene;
	cbTransformScene.mView = g_mView.Transpose();
	cbTransformScene.mProjection = g_mProjection.Transpose();
	g_pImmediateContext->UpdateSubresource(g_pCB_TransformWorld, 0, nullptr, &cbTransformScene, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCB_TransformWorld);							   // update된 Cbuffer를 device context가 개별 shader에 setting 해줌 		
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCB_TransformWorld);							   // view matrix를 PS에서 가져올 수 있음 

	CB_Lights cbLight;
	cbLight.posLight = Vector3::Transform(g_pos_light, g_mView);
	cbLight.dirLight = Vector3::TransformNormal(Vector3(0, 1, 0), g_mView);
	cbLight.dirLight.Normalize();
	cbLight.lightColor = Color(1, 1, 1, 1).RGBA();
	cbLight.lightFlag = 0;
	// 이런 경우에 가장 좋은 BUFFER는? USAGE는 무엇인가? **시험문제**
	// tempolar하게 dynamic usage resource를 만드는 것 -> CPU write이 되니까 dynamic resource에 값을 채워넣고 그렇게 저장된 GPU의 resource를 constantbuffer에 copy하면 GPU write이 된다 
	// constant buffer는 매 프레임마다 자주 바뀜, 그렇기 때문에 그 값을 update하기 위해서 tempoler하게 dynamic resource를 잡고 copy하는 것이 오히려 overhead가 걸릴 확률이 높음 -> 그런 경우에 대해서는 그냥 DYNAMIC으로 만들면 됨 
	// 그런데 constant buffer가 매 프레임이 아니라 간혹 일어나는 event에 대해서만 update가 된다면 DEFAULT가 더 낫다 
	g_pImmediateContext->UpdateSubresource(g_pCB_Lights, 0, nullptr, &cbLight, 0, 0);
	//g_pImmediateContext->VSSetConstantBuffers(2, 1, &g_pCB_Lights); 굉장히 작은 resource이지만 VS 에서는 해당 정보를 사용하지 않기 때문에 낭비하지말고! 
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCB_Lights);

	// shader resource에서 shader code를 보면 0번째 slot은 NormalBuffer가 쓰고 있음 
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_texEnvMap);

	// Just clear the backbuffer
	// draw buffer가(main rendering function) 호출되기 전에 buffer clear 
	// 이때 clear 할 buffer는 RGBA가 저장되어 있는 render target buffer, depth test를 수행할 depth buffer
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	// Clear the depth buffer to 1.0 (max depth) 
	// 이때 depth buffer는 최대 dapth value 1로 초기화 함으로써 1보다 가까운 projection space에서의 depth값을 갖는 pixel이 그려지게 할 것임 
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

#pragma endregion Common Scene (World)
	// C에서의 for-each 구문 
	// g_sceneObject : map, 두 개의 key-value pair 가 들어가 있음, key: string, value: MyObject 
	// auto -> std::pair<std::string, MyObject> -> 명시적으로 element를 갖고 있는, 어떤 typoe의 변수를 쓴다를 auto가 code의 흐름에 맞춰서 명시적인 것으로 compile time에 대체해줌 (편의성)

	for (auto& pair : g_sceneObjs)
	{
		//if (pair.first == "CUBE") continue;
		// pair.first = key, pair.second = value, MyObject, &ojb -> MyObject의 reference를 가져오도록 함, 전체 copy가 아닌 reference 참조만 하는 것임  
		// 그냥 obj로 하게 된다면 MyObject가 가지고 있는 모든 parameter value를 모두 copy하기 때문에 value에 대한 copy overhead가 생김, 물론 성능에는 큰 영향을 끼치진 않음 
		MyObject& obj = pair.second;

#pragma region For Each Object
		// Set vertex buffer
		UINT stride = obj.vb_stride;
		UINT offset = 0;
		g_pImmediateContext->IASetInputLayout(obj.pIALayer);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &obj.pVBuffer, &stride, &offset); // g_pVertexBuffer_sphere
		// Set index buffer
		// index buffer의 stide, size를 MyObject의 variable로 추가, size에 따라서 input assembler의 index buffer의 size를 명시적으로 set함 
		switch (obj.ib_stride)  
		{
		case 2:
			g_pImmediateContext->IASetIndexBuffer(obj.pIBuffer, DXGI_FORMAT_R16_UINT, 0); break; // cube model의 index buffer는 16bit unsigned int를 사용 
		case 4:
			g_pImmediateContext->IASetIndexBuffer(obj.pIBuffer, DXGI_FORMAT_R32_UINT, 0); break; // sphere model은 index buffer로 32bit unsigned int 사용 -> 4byte unsigned int, 충분한 개수를 보장하기 위함
		default:
			break;
		}
		// Set primitive topology
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// object가 material properties와 Wmatrix를 따로 갖음 
		// 그리고 경우에 따라서 Rendering 전에 다른 logic에서 Wmatrix, material properties가 바뀌는 경우가 존재할 수 있기 때문에 바뀌는 경우 바뀐 값이 적용되기 위해 UpdateConstantBuffer 호출 
		obj.UpdateConstanceBuffer();
		ID3D11Buffer* pCBuffer = obj.GetConstantBuffer();
		g_pImmediateContext->VSSetConstantBuffers(1, 1, &pCBuffer);
		g_pImmediateContext->PSSetConstantBuffers(1, 1, &pCBuffer);

		// SRViewNormal이 null이 아니면 shader에 Normal resource buffer set
		if (obj.pSRViewNormal)
			g_pImmediateContext->PSSetShaderResources(0, 1, &obj.pSRViewNormal);

		// model별로 RSS, DSS를 갖게 함 
		g_pImmediateContext->RSSetState(obj.pRSState);
		g_pImmediateContext->OMSetDepthStencilState(obj.pDSState, 0);

		// Render a triangle
		// object별로 VS, PS로 갖게 함
		// 특히 VS는 cube, sphere model이 다른 input layer, Vertex attribute를 갖고 있음, position, normal, color, ~ texture~ 
		// 그렇기 때문에 model마다 다르게 바뀌어야 함 
		// PS는 같은 것을 사용할 수도 있음, 그렇지만 object 단위로 설정함 
		g_pImmediateContext->VSSetShader(obj.pVShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(obj.pPShader, nullptr, 0);
		
		// 만약 index buffer가 설정되어 있다면 indexed function을 
		// 그렇지 않으면 draw function을 사용 
		// draw function을 쓸때는 vertex buffer의 element 개수를 적고, index buffer를 쓸 때는 index buffer의 element개수를 적음 
		// 그래서 object 별로 다른 개수가 들어갈 수 있기 때문에 MyObject variable로 별도로 저장해 사용함

		// 36개씩 cube를 그리고 있기 때문에 어떤 face를 그리고 있는지 확인하고 싶으면 offset 6단위로 확인!  
		// 이때 culling에 의해 뒷 부분은 보이지 않으므로 descRaster.CullMode = D3D11_CULL_BACK -> NONE으로 수정   
		//g_pImmediateContext->DrawImndexed(6,0,0);
		if (obj.pIBuffer)
			g_pImmediateContext->DrawIndexed(obj.drawCount, 0, 0);
		else 
			g_pImmediateContext->Draw(obj.drawCount, 0);
	}
#pragma endregion For Each Object

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	// map 에서 constant buffer를 생성하고 있음 
	// 그래서 그냥 종료하면, map이 소멸하게 되면 delete function이 자동으로 호출되지 않음 
	// 그렇기 때문에 별도로 다음과 같이 map에 들어있는 모든 element에 대해서 MyObject의 Delete를 호출함
	// 다 호출한 후 resource를 release 한 후 해당 map을 clear 함 
	for (auto& obj : g_sceneObjs) obj.second.Delete();
	g_sceneObjs.clear(); 

	// global variable로 GPU resource에 할당된 것들을 모두 Release 
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pCB_TransformWorld) g_pCB_TransformWorld->Release();
	if (g_pCB_Lights) g_pCB_Lights->Release();

	if (g_pIndexBuffer_cube) g_pIndexBuffer_cube->Release();
	if (g_pIndexBuffer_stl) g_pIndexBuffer_stl->Release();
	if (g_pIndexBuffer_sphere) g_pIndexBuffer_sphere->Release();

	if (g_pVertexBuffer_cube) g_pVertexBuffer_cube->Release();
	if (g_pSRV_cube) g_pSRV_cube->Release();
	if (g_pNormalBuffer_cube) g_pNormalBuffer_cube->Release();

	if (g_pVertexBuffer_stl) g_pVertexBuffer_stl->Release();
	if (g_pSRV_stl) g_pSRV_stl->Release();
	if (g_pNormalBuffer_stl) g_pNormalBuffer_stl->Release();

	if (g_pVertexBuffer_sphere) g_pVertexBuffer_sphere->Release();
	
	if (g_pIALayoutPCN) g_pIALayoutPCN->Release();
	if (g_pIALayoutPNT) g_pIALayoutPNT->Release();
	if (g_pIALayoutP) g_pIALayoutP->Release();
	if (g_pVertexShaderPCN) g_pVertexShaderPCN->Release();
	if (g_pVertexShaderPNT) g_pVertexShaderPNT->Release();
	if (g_pVertexShaderP) g_pVertexShaderP->Release();
	if (g_pPixelShader1) g_pPixelShader1->Release();
	if (g_pPixelShader2) g_pPixelShader2->Release();

	if (g_texEnvMap) g_texEnvMap->Release();
	if (g_tSRVEnvMap) g_tSRVEnvMap->Release();

	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pRSState) g_pRSState->Release();
	if (g_pDSState) g_pDSState->Release();

	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}