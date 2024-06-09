#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <basetsd.h>
#include <math.h>
#include <stdio.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <tchar.h>
#include <dinput.h>
#include "../Framework/dxutil.h"
#include "../Framework/D3DEnumeration.h"
#include "../Framework/D3DSettings.h"
#include "../Framework/D3DApp.h"
#include "../Framework/D3DFont.h"
#include "../Framework/D3DFile.h"
#include "../Framework/D3DUtil.h"
#include "../Framework/resource.h"
#include "DisplacementCompression.h"
#include <stack>

CMyD3DApplication* g_pApp = NULL;
HINSTANCE          g_hInst = NULL;

enum BarycentricPrecision
{
	BYTE_BARYCENTRIC,
	WORD_BARYCENTRIC,
	FLOAT_BARYCENTRIC,
};

const BarycentricPrecision gBarycentricPrecision = FLOAT_BARYCENTRIC;

#define DISPLACEMENT_MAP_SIZE 256

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	CMyD3DApplication d3dApp;
	g_pApp = &d3dApp;
	g_hInst = hInst;
	InitCommonControls();
	if (FAILED(d3dApp.Create(hInst)))
		return 0;
	return d3dApp.Run();
}

CMyD3DApplication::CMyD3DApplication()
{
	m_dwCreationWidth = 500;
	m_dwCreationHeight = 375;
	m_strWindowTitle = TEXT("DisplacementCompression");
	m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
	m_bStartFullscreen = false;
	m_bShowCursorWhenFullscreen = false;

	// d3dfont.cpp를 사용하여 D3D 글꼴 만들기
	m_pFont = new CD3DFont(_T("Arial"), 12, D3DFONT_BOLD);
	m_bLoadingApp = TRUE;
	m_pD3DXMesh = NULL;
	m_pD3DXPatchMesh = NULL;
	m_pD3DXPatchMeshDest = NULL;
	m_pDI = NULL;
	m_pKeyboard = NULL;

	m_pLinearDecl = NULL;
	m_linearPos = NULL;
	m_linearNorm = NULL;
	m_pVB = NULL;

	m_posVertexData = NULL;
	m_normVertexData = NULL;
	m_indexData = NULL;
	m_DisplacementTexture = NULL;

	m_numVertices = 0;
	m_numIndices = 0;

	ZeroMemory(&m_UserInput, sizeof(m_UserInput));
	m_fWorldRotX = 0.0f;
	m_fWorldRotY = 0.0f;

	m_displayMethod = DM_LINEAR;
	m_lod = 5;

	// 레지스트리에서 설정 읽기
	ReadSettings();
	GeneratePerlinNoisePattern(DISPLACEMENT_MAP_SIZE);
}

CMyD3DApplication::~CMyD3DApplication()
{
}

HRESULT CMyD3DApplication::OneTimeSceneInit()
{
	// TODO: 일회성 초기화 수행
	// 앱 로딩이 완료될 때까지 로딩 상태 메시지 그리기
	SendMessage(m_hWnd, WM_PAINT, 0, 0);

	// DirectInput 초기화
	InitInput(m_hWnd);
	m_bLoadingApp = FALSE;
	return S_OK;
}

VOID CMyD3DApplication::ReadSettings()
{
	HKEY hkey;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, DXAPP_KEY,0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
	{
		// TODO: 필요에 따라 변경

		// 저장된 창 너비/높이를 읽습니다.  이것은 단지 예일 뿐이며,
		// DXUtil_Read*() 함수를 사용하는 방법입니다.
		DXUtil_ReadIntRegKey(hkey, TEXT("Width"), &m_dwCreationWidth, m_dwCreationWidth);
		DXUtil_ReadIntRegKey(hkey, TEXT("Height"), &m_dwCreationHeight, m_dwCreationHeight);

		RegCloseKey(hkey);
	}
}

VOID CMyD3DApplication::WriteSettings()
{
	HKEY hkey;

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, DXAPP_KEY,0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
	{
		// TODO: 필요에 따라 변경

		// 창 너비/높이를 씁니다.
		// DXUtil_Write*() 함수를 사용하는 방법입니다.
		DXUtil_WriteIntRegKey(hkey, TEXT("Width"), m_rcWindowClient.right);
		DXUtil_WriteIntRegKey(hkey, TEXT("Height"), m_rcWindowClient.bottom);

		RegCloseKey(hkey);
	}
}

HRESULT CMyD3DApplication::InitInput(HWND hWnd)
{
	HRESULT hr;

	// IDirectInput8* 생성
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&m_pDI, NULL)))
		return DXTRACE_ERR("DirectInput8Create", hr);

	// 키보드용 IDirectInputDevice8*을 생성합니다.
	if (FAILED(hr = m_pDI->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, NULL)))
		return DXTRACE_ERR("CreateDevice", hr);

	// 키보드 데이터 형식을 설정합니다.
	if (FAILED(hr = m_pKeyboard->SetDataFormat(&c_dfDIKeyboard)))
		return DXTRACE_ERR("SetDataFormat", hr);

	// 키보드에서 협동 수준을 설정합니다.
	if (FAILED(hr = m_pKeyboard->SetCooperativeLevel(hWnd,
		DISCL_NONEXCLUSIVE |
		DISCL_FOREGROUND |
		DISCL_NOWINKEY)))
		return DXTRACE_ERR("SetCooperativeLevel", hr);

	// 키보드 획득
	m_pKeyboard->Acquire();

	return S_OK;
}

// 설명: 장치 초기화 중에 호출됩니다. 이 코드는 디스플레이 장치를 확인합니다.
HRESULT CMyD3DApplication::ConfirmDevice(D3DCAPS9* pCaps, DWORD dwBehavior,	D3DFORMAT Format)
{
	UNREFERENCED_PARAMETER(Format);
	UNREFERENCED_PARAMETER(dwBehavior);
	UNREFERENCED_PARAMETER(pCaps);

	BOOL bCapsAcceptable;

	// TODO: 이러한 디스플레이 캡이 허용되는지 확인하기 위해 검사를 수행합니다.
	//난 솔직히 이부분 모르겠음 왜 이따구로 코드작성하는지?=책에서 본부분 그대로 쓴건데..
	bCapsAcceptable = TRUE;
	if (bCapsAcceptable)
		return S_OK;
	else
		return E_FAIL;
}

HRESULT CMyD3DApplication::InitDeviceObjects()
{
	// TODO: 장치 객체 생성
	HRESULT hr;

	// 글꼴 초기화
	m_pFont->InitDeviceObjects(m_pd3dDevice);

	// D3DX를 사용하여 큐브 메시 생성
	if (FAILED(hr = D3DXCreateBox(m_pd3dDevice, 1, 1, 1, &m_pD3DXMesh, NULL)))
		return DXTRACE_ERR("D3DXCreateTeapot", hr);

	D3DXWELDEPSILONS Epsilons;
	memset(&Epsilons, 0, sizeof(D3DXWELDEPSILONS));
	Epsilons.Normal = 100000;
	D3DXWeldVertices(m_pD3DXMesh, 0, &Epsilons, 0, 0, 0, 0);

	// 평균 법선을 제공합니다(따라서 n - 패치가 더 흥미롭습니다).
	D3DXComputeNormals(m_pD3DXMesh, 0);
	GetXMeshData(m_pD3DXMesh);

	// 이 멋진 아이디어는 MS의 David Martin(D3DX 팀)이 제안한 것입니다.
	// 최대 배열 크기를 보장하기 전에 코드에서 정의를 설정했습니다.
	// 컴파일러 호출 :-) 이는 카드에 배열을 최대화할 수 있음을 의미합니다.
	// 기능
	D3DXMACRO pMacros[2];
	CHAR szNumConsts[10];

	// 컴파일러에 약간의 호흡 공간을 제공하기 위해 감소된 최대 상수 수
	D3DCAPS9 Caps;
	m_pd3dDevice->GetDeviceCaps(&Caps);
	const int NumConsts = Caps.MaxVertexShaderConst - 3; // 장치 캡 기준
	sprintf(szNumConsts, "%d", NumConsts);

	// #define NUM_CONSTS <n>
	pMacros[0].Name = "NUM_CONSTS";
	pMacros[0].Definition = szNumConsts;

	// NULL로 배열 종료
	pMacros[1].Name = NULL;
	pMacros[1].Name = NULL;

	// 선형
	if (FAILED(hr = D3DXCreateEffectFromFile(m_pd3dDevice, "linear.fx", pMacros, 0, 0, 0, &m_linearFX, 0)))
		return DXTRACE_ERR("D3DXCreateEffectFromFile", hr);

	// 기본적으로 선형 좌표 방법
	if (FAILED(hr = GenerateLinearConstants()))
		return DXTRACE_ERR("GenerateLinearConstants", hr);

	return S_OK;
}

HRESULT CMyD3DApplication::GetXMeshData(LPD3DXMESH in)
{
	HRESULT hRes;

	// 입력 정점 데이터를 얻습니다.
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if (FAILED(hRes)) return hRes;

	m_numVertices = in->GetNumVertices();

	// 정점을 저장하기 위해 일부 메모리를 할당합니다.
	m_posVertexData = new float[m_numVertices * 3];
	m_normVertexData = new float[m_numVertices * 3];
	m_uvVertexData = new float[m_numVertices * 2];

	// 정점 데이터를 복사합니다.
	for (unsigned int i = 0; i < m_numVertices; i++)
	{
		m_posVertexData[(i * 3) + 0] = inStream[0];
		m_posVertexData[(i * 3) + 1] = inStream[1];
		m_posVertexData[(i * 3) + 2] = inStream[2];
		m_normVertexData[(i * 3) + 0] = inStream[3];
		m_normVertexData[(i * 3) + 1] = inStream[4];
		m_normVertexData[(i * 3) + 2] = inStream[5];

		// 변위 맵을 적용하기 위해 구형 uv를 생성합니다.
		m_uvVertexData[(i * 2) + 0] = 0.5f * (1.f + inStream[3]);
		m_uvVertexData[(i * 2) + 1] = 0.5f * (1.f - inStream[4]);

		// 다음 정점
		inStream += 6;
	}

	hRes = in->UnlockVertexBuffer();
	if (FAILED(hRes)) return hRes;

	// 이제 인덱스를 복사합니다.
	m_numIndices = in->GetNumFaces() * 3;

	// 16비트 인덱스만	
	const unsigned int ibsize = m_numIndices * sizeof(WORD);

	m_indexData = new WORD[m_numIndices];

	WORD* inIBStream = 0;
	hRes = in->LockIndexBuffer(D3DLOCK_READONLY, (void**)&inIBStream);
	if (FAILED(hRes)) return hRes;

	memcpy(m_indexData, inIBStream, ibsize);

	// 이제 두 인덱스 버퍼를 모두 잠금 해제합니다.
	hRes = in->UnlockIndexBuffer();
	if (FAILED(hRes)) return hRes;

	return S_OK;
}

HRESULT CMyD3DApplication::GenerateLinearConstants()
{
	HRESULT hRes;

	// 먼저 정점 데이터의 색인을 해제합니다.
	m_linearPos = new float[m_numVertices * 4];
	m_linearNorm = new float[m_numVertices * 4];

	for (unsigned int i = 0; i < m_numVertices; i++)
	{
		m_linearPos[(i * 4) + 0] = m_posVertexData[(i * 3) + 0];
		m_linearPos[(i * 4) + 1] = m_posVertexData[(i * 3) + 1];
		m_linearPos[(i * 4) + 2] = m_posVertexData[(i * 3) + 2];
		m_linearPos[(i * 4) + 3] = 0;

		m_linearNorm[(i * 4) + 0] = m_normVertexData[(i * 3) + 0];
		m_linearNorm[(i * 4) + 1] = m_normVertexData[(i * 3) + 1];
		m_linearNorm[(i * 4) + 2] = m_normVertexData[(i * 3) + 2];
		m_linearNorm[(i * 4) + 3] = 0;
	}

	// 정점 데이터를 직접 사용합니다.
	hRes = GenerateLinearBuffers(m_lod);
	if (FAILED(hRes)) return hRes;

	D3DVERTEXELEMENT9	Decl[MAX_FVF_DECL_SIZE];

	// 선언 D3D 생성 
	// 무게 중심 좌표
	Decl[0].Stream = 0;
	Decl[0].Offset = 0;
	if (gBarycentricPrecision == BYTE_BARYCENTRIC)
		Decl[0].Type = D3DDECLTYPE_D3DCOLOR;
	else if (gBarycentricPrecision == WORD_BARYCENTRIC)
		Decl[0].Type = D3DDECLTYPE_USHORT4N;
	else if (gBarycentricPrecision == FLOAT_BARYCENTRIC)
		Decl[0].Type = D3DDECLTYPE_FLOAT2;
	
	Decl[0].Method = D3DDECLMETHOD_DEFAULT;
	Decl[0].Usage = D3DDECLUSAGE_POSITION;
	Decl[0].UsageIndex = 0;

	// 인덱스
	Decl[1].Stream = 0;
	if (gBarycentricPrecision == FLOAT_BARYCENTRIC)
		Decl[1].Offset = sizeof(float) * 2;
	else
		Decl[1].Offset = sizeof(unsigned int);

	Decl[1].Type = D3DDECLTYPE_D3DCOLOR;
	Decl[1].Method = D3DDECLMETHOD_DEFAULT;
	Decl[1].Usage = D3DDECLUSAGE_POSITION;
	Decl[1].UsageIndex = 1;

	// D3DDECL_END()
	Decl[2].Stream = 0xFF;
	Decl[2].Offset = 0;
	Decl[2].Type = D3DDECLTYPE_UNUSED;
	Decl[2].Method = 0;
	Decl[2].Usage = 0;
	Decl[2].UsageIndex = 0;

	SAFE_RELEASE(m_pLinearDecl);
	hRes = m_pd3dDevice->CreateVertexDeclaration(Decl, &m_pLinearDecl);
	if (FAILED(hRes)) return hRes;

	// 상수 정점을 설정합니다.
	m_linearFX->SetVectorArray("VertexPos", (D3DXVECTOR4*)m_linearPos, m_numVertices);
	m_linearFX->SetVectorArray("VertexNorm", (D3DXVECTOR4*)m_linearNorm, m_numVertices);

	return S_OK;
}

static unsigned int calcPatchVertsPerOrigTri(unsigned int n)
{
	return unsigned int(pow(4, n)) * 3;
}

struct BARYCENTRIC_COORDS //무게 중심 좌표
{
	float i, j; // k는 쉽게 계산할 수 있음

	BARYCENTRIC_COORDS() : i(0), j(0) {};
	BARYCENTRIC_COORDS(float x, float y) : i(x), j(y) {};
	BARYCENTRIC_COORDS(const BARYCENTRIC_COORDS& right) : i(right.i), j(right.j) {};
	float getI() { return i; };
	float getJ() { return j; };
	float getK() { return 1.f - i - j; };

	// i값을 바이트로 양자화
	BYTE getByteQuantisedI()
	{
		// 정밀도 부족으로 인한 구멍을 줄이기 위한 해킹
		unsigned int i = getI() * 256.f;
		if (i >= 256)
			i = 255;
		return BYTE(i);
	};
	BYTE getByteQuantisedJ()
	{
		// 정밀도 부족으로 인한 구멍을 줄이기 위한 해킹
		unsigned int j = getJ() * 256.f;
		if (j >= 256)
			j = 255;
		return BYTE(j);
	};
	BYTE getByteQuantisedK()
	{
		// 정밀도 부족으로 인한 구멍을 줄이기 위한 해킹
		unsigned int k = getK() * 256.f;
		if (k >= 256)
			k = 255;
		return BYTE(k);
	};
	WORD getWordQuantisedI()
	{
		unsigned int i = getI() * 65535.f;
		return WORD(i);
	};
	WORD getWordQuantisedJ()
	{
		unsigned int j = getJ() * 65535.f;
		return WORD(j);
	};
	WORD getWordQuantisedK()
	{
		unsigned int k = getK() * 65535.f;
		return WORD(k);
	};
	BARYCENTRIC_COORDS& operator+=(const BARYCENTRIC_COORDS& right)// 복사 대입 연산자
	{
		i = i + right.i;
		j = j + right.j;
		return *this;
	}
	BARYCENTRIC_COORDS operator+(const BARYCENTRIC_COORDS& right) const  // 덧셈 연산자
	{
		BARYCENTRIC_COORDS ret(*this);
		ret += right;
		return ret;
	}
	BARYCENTRIC_COORDS& operator/(float right)  // 나누기 연산자
	{
		i = i / right;
		j = j / right;
		return *this;
	}
};

struct BARYCENTRIC_TRIANGLE
{
	BARYCENTRIC_COORDS a, b, c;
	unsigned int lod;
	BARYCENTRIC_TRIANGLE() : a(1, 0), b(0, 1), c(0, 0), lod(0) {};
	BARYCENTRIC_TRIANGLE(const BARYCENTRIC_TRIANGLE& right) {
		a = right.a;
		b = right.b;
		c = right.c;
		lod = right.lod;
	}
};

//패치테셀레이트
static void PatchTesselate(BARYCENTRIC_TRIANGLE tri, BARYCENTRIC_TRIANGLE& a, BARYCENTRIC_TRIANGLE& b, BARYCENTRIC_TRIANGLE& c,	BARYCENTRIC_TRIANGLE& d)
{
	BARYCENTRIC_COORDS ab = (tri.a + tri.b) / 2;
	BARYCENTRIC_COORDS bc = (tri.b + tri.c) / 2;
	BARYCENTRIC_COORDS ac = (tri.a + tri.c) / 2;

	a.a = tri.a;
	a.b = ab;
	a.c = ac;
	a.lod = tri.lod + 1;

	b.a = bc;
	b.b = tri.c;
	b.c = ac;
	b.lod = tri.lod + 1;

	c.a = bc;
	c.b = ab;
	c.c = tri.b;
	c.lod = tri.lod + 1;

	d.a = bc;
	d.b = ac;
	d.c = ab;
	d.lod = tri.lod + 1;
}

//void CMyD3DApplication::CreateMandelbrot(unsigned int dwSize) //만델브로트 생성.. 
//{
//	// 흥미로운 것을 보기 위해 mandlebrot를 생성합니다.
//	float val;
//	float MinRe = -2.0f;
//	float MaxRe = 1.0f;
//	float MinIm = -1.2f;
//	float MaxIm = MinIm + (MaxRe - MinRe);
//	float ReFactor = (MaxRe - MinRe) / (dwSize - 1);
//	float ImFactor = (MaxIm - MinIm) / (dwSize - 1);
//
//	m_DisplacementTexture = new float[dwSize * dwSize];
//
//	for (DWORD z = 0; z < dwSize; z++) {
//		float cim = MaxIm - z * ImFactor;
//		for (DWORD x = 0; x < dwSize; x++) {
//			float cre = MinRe + x * ReFactor;
//			float zre = cre, zim = cim;
//			unsigned loop;
//			for (loop = 0; loop < 1000; loop++) { // iteration count increased for more detail
//				float zre2 = zre * zre;
//				float zim2 = zim * zim;
//				if (zre2 + zim2 > 4.0f) break; // escape condition
//				zim = 2 * zre * zim + cim;
//				zre = zre2 - zim2 + cre;
//			}
//			val = loop / 1000.0f; // normalize value to [0, 1]
//			m_DisplacementTexture[(z * dwSize) + x] = val;
//		}
//	}
//	delete m_DisplacementTexture;
//}

// 지정된 좌표에서의 변위 값을 조회합니다.
float CMyD3DApplication::LookupDisplacementValue(WORD i0, WORD i1, WORD i2, float i, float j)
{
	const float k = 1.f - i - j;
	float u, v;
	u = (m_uvVertexData[(i0 * 2) + 0] * i)
		+ (m_uvVertexData[(i1 * 2) + 0] * j)
		+ (m_uvVertexData[(i2 * 2) + 0] * k);
	v = (m_uvVertexData[(i0 * 2) + 1] * i)
		+ (m_uvVertexData[(i1 * 2) + 1] * j)
		+ (m_uvVertexData[(i2 * 2) + 1] * k);

	int address = (int)(u * DISPLACEMENT_MAP_SIZE * DISPLACEMENT_MAP_SIZE) + (v * DISPLACEMENT_MAP_SIZE);
	return m_DisplacementTexture[address];
}

float CMyD3DApplication::Fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float CMyD3DApplication::Lerp(float t, float a, float b)
{
	return a + t * (b - a);
}

float CMyD3DApplication::Grad(int hash, float x, float y)
{
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

void CMyD3DApplication::InitPermutation()
{
	for (int i = 0; i < 256; i++) {
		p[256 + i] = p[i] = permutation[i];
	}
}

float CMyD3DApplication::PerlinNoise(float x, float y)
{
	int X = (int)floor(x) & 255;
	int Y = (int)floor(y) & 255;

	x -= floor(x);
	y -= floor(y);

	float u = Fade(x);
	float v = Fade(y);

	int a = p[X] + Y;
	int aa = p[a];
	int ab = p[a + 1];
	int b = p[X + 1] + Y;
	int ba = p[b];
	int bb = p[b + 1];

	return Lerp(v, Lerp(u, Grad(p[aa], x, y), Grad(p[ba], x - 1, y)),Lerp(u, Grad(p[ab], x, y - 1), Grad(p[bb], x - 1, y - 1)));
}

void CMyD3DApplication::GeneratePerlinNoisePattern(DWORD dwSize)
{
	InitPermutation();

	float frequency = 5.0f;
	float amplitude = 1.0f;

	m_DisplacementTexture = new float[dwSize * dwSize];

	for (DWORD z = 0; z < dwSize; z++) {
		for (DWORD x = 0; x < dwSize; x++) {
			float xf = (float)x / dwSize * frequency;
			float zf = (float)z / dwSize * frequency;
			float val = PerlinNoise(xf, zf) * amplitude;
			m_DisplacementTexture[(z * dwSize) + x] = val;
		}
	}

	delete[] m_DisplacementTexture;
}

HRESULT CMyD3DApplication::GenerateLinearBuffers(unsigned int LOD)
{
	HRESULT hRes;

	m_lod = LOD;

	SAFE_RELEASE(m_pVB);

	const unsigned int numTri = m_numIndices / 3;
	const unsigned int numVerts = calcPatchVertsPerOrigTri(LOD) * numTri;
	unsigned int vbSize;

	// 무게 중심은 모든 표면에서 재사용 가능
	// 분할 금액이 비슷하므로 2개 스트림으로 분할하면 많은 비용을 절약할 수 있습니다.
	// 램
	if (gBarycentricPrecision == FLOAT_BARYCENTRIC)
		vbSize = (sizeof(float) * 2 + sizeof(unsigned int)) * numVerts;
	else
		vbSize = (sizeof(unsigned int) * 2) * numVerts;

	hRes = m_pd3dDevice->CreateVertexBuffer(vbSize, 0, 0, D3DPOOL_MANAGED, &m_pVB, NULL);
	if (FAILED(hRes)) return hRes;

	unsigned int* outStream;

	hRes = m_pVB->Lock(0, vbSize, (void**)&outStream, 0);
	if (FAILED(hRes)) return hRes;

	for (unsigned int index = 0; index < m_numIndices / 3; index++)
	{
		std::stack<BARYCENTRIC_TRIANGLE> lodStack;
		lodStack.push(BARYCENTRIC_TRIANGLE());

		while (!lodStack.empty())
		{
			BARYCENTRIC_TRIANGLE tri = lodStack.top();
			lodStack.pop();

			if (tri.lod < LOD)
			{
				BARYCENTRIC_TRIANGLE tri_a, tri_b, tri_c, tri_d;
				PatchTesselate(tri, tri_a, tri_b, tri_c, tri_d);
				lodStack.push(tri_a);
				lodStack.push(tri_b);
				lodStack.push(tri_c);
				lodStack.push(tri_d);
			}
			else
			{
				if (tri.lod == LOD)
				{
					unsigned int uii, uij;
					// 바이트 인덱스가 문제가 되지 않도록 큰 메시를 분할해야 한다는 점을 기억하세요(현재)
					BYTE i0 = (BYTE)m_indexData[(index * 3) + 0];
					BYTE i1 = (BYTE)m_indexData[(index * 3) + 1];
					BYTE i2 = (BYTE)m_indexData[(index * 3) + 2];

					float d0 = LookupDisplacementValue(i0, i1, i2, tri.a.getI(), tri.a.getJ()) * 255;
					float d1 = LookupDisplacementValue(i0, i1, i2, tri.b.getI(), tri.b.getJ()) * 255;
					float d2 = LookupDisplacementValue(i0, i1, i2, tri.c.getI(), tri.c.getJ()) * 255;


					if (gBarycentricPrecision == FLOAT_BARYCENTRIC)
					{
						*((float*)outStream + 0) = tri.a.getI();
						*((float*)outStream + 1) = tri.a.getJ();
						*(outStream + 2) = ((unsigned int)(d0) << 24) | (i0 << 16) | (i1 << 8) | (i2 << 0);
						*((float*)outStream + 3) = tri.b.getI();
						*((float*)outStream + 4) = tri.b.getJ();
						*(outStream + 5) = ((unsigned int)(d1) << 24) | (i0 << 16) | (i1 << 8) | (i2 << 0);
						*((float*)outStream + 6) = tri.c.getI();
						*((float*)outStream + 7) = tri.c.getJ();
						*(outStream + 8) = ((unsigned int)(d2) << 24) | (i0 << 16) | (i1 << 8) | (i2 << 0);

						outStream += 9;
					}
					else
					{
						// 출력 좌표
						if (gBarycentricPrecision == BYTE_BARYCENTRIC)
						{
							uii = tri.a.getByteQuantisedI();
							uij = tri.a.getByteQuantisedJ();
							*(outStream + 0) = (uii << 16) | (uij << 8); // D3DCOLOR 압축 풀기에 적합
							uii = tri.b.getByteQuantisedI();
							uij = tri.b.getByteQuantisedJ();
							*(outStream + 2) = (uii << 16) | (uij << 8); // D3DCOLOR 압축 풀기에 적합
							uii = tri.c.getByteQuantisedI();
							uij = tri.c.getByteQuantisedJ();
							*(outStream + 4) = (uii << 16) | (uij << 8); // D3DCOLOR 압축 풀기에 적합
						}
						else if (gBarycentricPrecision == WORD_BARYCENTRIC)
						{
							uii = tri.a.getWordQuantisedI();
							uij = tri.a.getWordQuantisedJ();
							*(outStream + 0) = (uii << 0) | (uij << 16);
							uii = tri.b.getWordQuantisedI();
							uij = tri.b.getWordQuantisedJ();
							*(outStream + 2) = (uii << 0) | (uij << 16);
							uii = tri.c.getWordQuantisedI();
							uij = tri.c.getWordQuantisedJ();
							*(outStream + 4) = (uii << 0) | (uij << 16);
						}
						*(outStream + 1) = (i0 << 16) | (i1 << 8) | (i2 << 0);
						*(outStream + 3) = (i0 << 16) | (i1 << 8) | (i2 << 0);
						*(outStream + 5) = (i0 << 16) | (i1 << 8) | (i2 << 0);
						outStream += 6;
					}
				}
			}
		}
	}

	// 이제 정점 버퍼의 잠금을 해제합니다.
	hRes = m_pVB->Unlock();
	if (FAILED(hRes)) return hRes;

	return S_OK;
}

HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	// TODO: 렌더 상태 설정
	// 머티리얼 설정
	D3DMATERIAL9 mtrl;
	D3DUtil_InitMaterial(mtrl, 1.0f, 0.0f, 0.0f);
	m_pd3dDevice->SetMaterial(&mtrl);

	// 텍스처 설정
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	m_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	// 기타 렌더링 상태 설정
	m_pd3dDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x000F0F0F);

	// 월드 매트릭스 설정
	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);
	m_pd3dDevice->SetTransform(D3DTS_WORLD, &matIdentity);
	m_matWorld = matIdentity;

	// 뷰 매트릭스를 설정합니다. 뷰 매트릭스는 시점을 기준으로 정의할 수 있습니다.
	// 볼 지점과 위쪽 방향입니다. 여기서는
	// z축을 따라 뒤로 5단위, 위쪽으로 3단위를 살펴봅니다.
	// 원점을 지정하고 "위쪽"이 y 방향이 되도록 정의합니다.
	D3DXMATRIX matView;
	D3DXVECTOR3 vFromPt = D3DXVECTOR3(0.0f, 0.0f, -3.0f);
	D3DXVECTOR3 vLookatPt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &vFromPt, &vLookatPt, &vUpVec);
	m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
	m_matView = matView;

	// 투영 행렬 설정
	D3DXMATRIX matProj;
	FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, fAspect, 1.0f, 100.0f);
	m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
	m_matProj = matProj;

	// 조명 상태 설정
	D3DLIGHT9 light;
	D3DUtil_InitLight(light, D3DLIGHT_DIRECTIONAL, -1.0f, -1.0f, 2.0f);
	m_pd3dDevice->SetLight(0, &light);
	m_pd3dDevice->LightEnable(0, TRUE);
	m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	// 글꼴 복원
	m_pFont->RestoreDeviceObjects();

	return S_OK;
}

HRESULT CMyD3DApplication::FrameMove()
{
	// 사용자 입력 상태 업데이트
	UpdateInput(&m_UserInput);

	// 사용자 입력에 따라 세계 상태를 업데이트합니다.
	D3DXMATRIX matWorld;
	D3DXMATRIX matRotY;
	D3DXMATRIX matRotX;

	if (m_UserInput.bRotateLeft && !m_UserInput.bRotateRight)
		m_fWorldRotY += m_fElapsedTime;
	else if (m_UserInput.bRotateRight && !m_UserInput.bRotateLeft)
		m_fWorldRotY -= m_fElapsedTime;

	if (m_UserInput.bRotateUp && !m_UserInput.bRotateDown)
		m_fWorldRotX += m_fElapsedTime;
	else if (m_UserInput.bRotateDown && !m_UserInput.bRotateUp)
		m_fWorldRotX -= m_fElapsedTime;

	D3DXMatrixRotationX(&matRotX, m_fWorldRotX);
	D3DXMatrixRotationY(&matRotY, m_fWorldRotY);

	D3DXMatrixMultiply(&matWorld, &matRotX, &matRotY);
	m_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
	m_matWorld = matWorld;

	// LOD 선택(사용 가능한 경우)
	if (m_UserInput.b1)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(0);

	else if (m_UserInput.b2)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(1);
	
	else if (m_UserInput.b3)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(2);
	
	else if (m_UserInput.b4)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(3);
		
	else if (m_UserInput.b5)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(4);
	
	else if (m_UserInput.b6)
		if (m_displayMethod == DM_LINEAR)
			GenerateLinearBuffers(5);

	// 메소드 선택
	if (m_UserInput.bf5)
	{
	}
	else if (m_UserInput.bf6)
	{
	}
	else if (m_UserInput.bf7)
	{
	}
	else if (m_UserInput.bf8)
	{
	}
	else if (m_UserInput.bf9 && m_displayMethod != DM_LINEAR)
	{
		m_displayMethod = DM_LINEAR;
		GenerateLinearBuffers(m_lod);
	}

	return SetupDisplayMethod();
}

void CMyD3DApplication::UpdateInput(UserInput* pUserInput)
{
	HRESULT hr;

	// 입력의 장치 상태를 가져오고 상태를 흐리게 표시합니다.
	ZeroMemory(&pUserInput->diks, sizeof(pUserInput->diks));
	hr = m_pKeyboard->GetDeviceState(sizeof(pUserInput->diks), &pUserInput->diks);
	if (FAILED(hr))
	{
		m_pKeyboard->Acquire();
		return;
	}

	pUserInput->bRotateLeft = ((pUserInput->diks[DIK_LEFT] & 0x80) == 0x80);
	pUserInput->bRotateRight = ((pUserInput->diks[DIK_RIGHT] & 0x80) == 0x80);
	pUserInput->bRotateUp = ((pUserInput->diks[DIK_UP] & 0x80) == 0x80);
	pUserInput->bRotateDown = ((pUserInput->diks[DIK_DOWN] & 0x80) == 0x80);
	pUserInput->b1 = ((pUserInput->diks[DIK_1] & 0x80) == 0x80);
	pUserInput->b2 = ((pUserInput->diks[DIK_2] & 0x80) == 0x80);
	pUserInput->b3 = ((pUserInput->diks[DIK_3] & 0x80) == 0x80);
	pUserInput->b4 = ((pUserInput->diks[DIK_4] & 0x80) == 0x80);
	pUserInput->b5 = ((pUserInput->diks[DIK_5] & 0x80) == 0x80);
	pUserInput->b6 = ((pUserInput->diks[DIK_6] & 0x80) == 0x80);
	pUserInput->b7 = ((pUserInput->diks[DIK_7] & 0x80) == 0x80);
	pUserInput->b8 = ((pUserInput->diks[DIK_8] & 0x80) == 0x80);
	pUserInput->b9 = ((pUserInput->diks[DIK_9] & 0x80) == 0x80);
	pUserInput->bf5 = ((pUserInput->diks[DIK_F5] & 0x80) == 0x80);
	pUserInput->bf6 = ((pUserInput->diks[DIK_F6] & 0x80) == 0x80);
	pUserInput->bf7 = ((pUserInput->diks[DIK_F7] & 0x80) == 0x80);
	pUserInput->bf8 = ((pUserInput->diks[DIK_F8] & 0x80) == 0x80);
	pUserInput->bf9 = ((pUserInput->diks[DIK_F9] & 0x80) == 0x80);
}

HRESULT CMyD3DApplication::Render()
{
	// 뷰포트 지우기
	m_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		0x50505050, 1.0f, 0L);

	m_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	// 장면 시작
	if (SUCCEEDED(m_pd3dDevice->BeginScene()))
	{
		// MVP 계산
		D3DXMATRIX matMVP;
		D3DXMatrixMultiply(&matMVP, &m_matView, &m_matProj);
		D3DXMatrixMultiply(&matMVP, &m_matWorld, &matMVP);

		// 공유 효과 풀을 사용해야 하지만 이는 단지 데모일 뿐입니다...
		m_linearFX->SetMatrix("MVP", &matMVP);

		unsigned int numPasses;

		switch (m_displayMethod)
		{
		case DM_LINEAR:
			m_linearFX->Begin(&numPasses, 0);
			for (unsigned int i = 0; i < numPasses; i++)
			{
				m_pd3dDevice->SetVertexDeclaration(m_pLinearDecl);
				m_linearFX->BeginPass(i);
				const unsigned int numTri = m_numIndices / 3;
				unsigned int numVerts = calcPatchVertsPerOrigTri(m_lod) * numTri;

				unsigned int vSize;
				if (gBarycentricPrecision == FLOAT_BARYCENTRIC)
					vSize = sizeof(float) * 2 + sizeof(unsigned int);
				
				else
					vSize = sizeof(unsigned int) * 2;

				// 일부 그래픽카드는 한 번에 많은 정점을 처리하지 못합니다.
				if (numVerts > 65536)
					numVerts = 65536;

				m_pd3dDevice->SetStreamSource(0, m_pVB, 0, vSize);
				m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, numVerts / 3);
			}
			m_linearFX->EndPass();
			break;
		}

		// 렌더링 통계 및 도움말 텍스트
		RenderText();

		// 장면을 종료합니다.
		m_pd3dDevice->EndScene();
	}

	return S_OK;
}

HRESULT CMyD3DApplication::RenderText()
{
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 0);
	TCHAR szMsg[MAX_PATH] = TEXT("");

	// 디스플레이 통계 출력
	FLOAT fNextLine = 40.0f;

	lstrcpy(szMsg, m_strDeviceStats);
	fNextLine -= 20.0f;
	m_pFont->DrawText(2, fNextLine, fontColor, szMsg);

	lstrcpy(szMsg, m_strFrameStats);
	fNextLine -= 20.0f;
	m_pFont->DrawText(2, fNextLine, fontColor, szMsg);

	// 통계 및 도움말 출력
	fNextLine = (FLOAT)m_d3dsdBackBuffer.Height;
	wsprintf(szMsg, TEXT("Arrow keys: Up=%d Down=%d Left=%d Right=%d"), m_UserInput.bRotateUp, m_UserInput.bRotateDown, m_UserInput.bRotateLeft, m_UserInput.bRotateRight);
	fNextLine -= 20.0f; m_pFont->DrawText(2, fNextLine, fontColor, szMsg);
	lstrcpy(szMsg, TEXT("Use arrow keys to rotate object"));
	fNextLine -= 20.0f; m_pFont->DrawText(2, fNextLine, fontColor, szMsg);
	lstrcpy(szMsg, TEXT("Press 'F2' to configure display"));
	fNextLine -= 20.0f; m_pFont->DrawText(2, fNextLine, fontColor, szMsg);
	return S_OK;
}

LRESULT CMyD3DApplication::MsgProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		if (m_bLoadingApp)
		{
			// 창에 그림을 그려 사용자에게 앱이 로드 중임을 알립니다.
			// TODO: 필요에 따라 변경
			HDC hDC = GetDC(hWnd);
			TCHAR strMsg[MAX_PATH];
			wsprintf(strMsg, TEXT("로딩중...기다리세요"));
			RECT rct;
			GetClientRect(hWnd, &rct);
			DrawText(hDC, strMsg, -1, &rct, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			ReleaseDC(hWnd, hDC);
		}
		break;
	}

	}

	return CD3DApplication::MsgProc(hWnd, msg, wParam, lParam);
}

HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
	// TODO: RestoreDeviceObjects()에서 생성된 모든 개체를 정리합니다.
	m_pFont->InvalidateDeviceObjects();

	return S_OK;
}

HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	CleanDisplayMethod();

	delete m_posVertexData; m_posVertexData = 0;
	delete m_normVertexData; m_normVertexData = 0;
	delete m_uvVertexData; m_uvVertexData = 0;
	delete m_indexData; m_indexData = 0;
	m_numVertices = 0;
	m_numIndices = 0;

	m_pFont->DeleteDeviceObjects();

	SAFE_RELEASE(m_linearFX);

	SAFE_RELEASE(m_pD3DXPatchMeshDest);
	SAFE_RELEASE(m_pD3DXPatchMesh);
	SAFE_RELEASE(m_pD3DXMesh);
	return S_OK;
}

void CMyD3DApplication::CleanDisplayMethod()
{
	SAFE_RELEASE(m_pVB);

	SAFE_RELEASE(m_pLinearDecl);

	delete m_linearPos; m_linearPos = 0;
	delete m_linearNorm; m_linearNorm = 0;
}

HRESULT CMyD3DApplication::SetupDisplayMethod()
{
	HRESULT hr;

	if (m_displayMethod == DM_LINEAR)
	{
		if (m_pLinearDecl == 0)
		{
			CleanDisplayMethod();
			if (FAILED(hr = GenerateLinearConstants()))
				return DXTRACE_ERR("GenerateLinearConstants", hr);
		}
	}
	return S_OK;
}

HRESULT CMyD3DApplication::FinalCleanup()
{
	delete[] m_DisplacementTexture;

	// TODO: 필요한 최종 정리를 수행합니다.
	// D3D 글꼴 정리
	SAFE_DELETE(m_pFont);

	// DirectInput 정리
	CleanupDirectInput();

	// 레지스트리에 설정을 씁니다.
	WriteSettings();

	return S_OK;
}

VOID CMyD3DApplication::CleanupDirectInput()
{
	// DirectX 입력 객체 정리
	SAFE_RELEASE(m_pKeyboard);
	SAFE_RELEASE(m_pDI);
}