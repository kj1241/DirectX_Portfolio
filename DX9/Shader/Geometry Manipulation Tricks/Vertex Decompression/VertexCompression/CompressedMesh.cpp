//버텍스 코덱의 코드
#define STRICT
#include <windows.h>
#include <basetsd.h>
#include <math.h>
#include <stdio.h>
#include <cassert>
#include <D3D9types.h>
#include <D3DX9.h>
#include <DXErr.h>
#include "../Framework/DXUtil.h"
#include "../Framework/D3DUtil.h"
#include "../Framework/D3DEnumeration.h"
#include "../Framework/D3DSettings.h"

#include "CompressedMesh.h"

// 고유 솔버(eigenvector.cpp 삼중대각행렬 끌고와야됨)
extern void tri_diag(double a[], double d[], double e[], double z[], int n,double tol);
extern int calc_eigenstructure(double d[],double e[],double z[], int n, double macheps);

CompressedMesh::CompressedMesh(  LPDIRECT3DDEVICE9 pD3D ) : 
	m_pD3D(pD3D),
	m_pVB(0),
	m_pIB(0),
	m_pDecl(0),
	m_vertexSize(0),
	m_numVertex(0),
	m_numIndices(0)
{
}

CompressedMesh::~CompressedMesh()
{
	SAFE_RELEASE( m_pIB );
	SAFE_RELEASE( m_pVB );
	SAFE_RELEASE( m_pDecl );
}

//d3d 정점 유형의 크기(바이트)
//주어진 Direct3D 정점 유형의 크기를 바이트 단위로 반환합니다.
//switch 문을 사용하여 다양한 정점 유형에 따라 적절한 크기를 계산합니다.
unsigned int CompressedMesh::SizeOfD3DVertexType( const unsigned int type )
{
	switch(type)
	{
	case D3DDECLTYPE_FLOAT1:
		return sizeof(float);
	case D3DDECLTYPE_FLOAT2:
		return sizeof(float) * 2;
	case D3DDECLTYPE_FLOAT3:
		return sizeof(float) * 3;
	case D3DDECLTYPE_FLOAT4:
		return sizeof(float) * 4;
	case D3DDECLTYPE_D3DCOLOR:
		return sizeof(unsigned char) * 4;
	case D3DDECLTYPE_UBYTE4:
		return sizeof(unsigned char) * 4;
	case D3DDECLTYPE_SHORT2:
		return sizeof(short) * 2;
	case D3DDECLTYPE_SHORT4:
		return sizeof(short) * 4;
	// 다음 유형은 정점 셰이더 >= 2.0에서만 유효합니다.
	// Dx9 도큐먼트에서 잘못 나옴... MS, ATI 및 NVdia로 TODO 확인
	case D3DDECLTYPE_UBYTE4N:	// 4바이트를 각각 255.0으로 나누어 정규화
		return sizeof(unsigned char) * 4;
	case D3DDECLTYPE_SHORT2N:	// 2D 부호 있는 짧은 정규화(v[0]/32767.0,v[1]/32767.0,0,1)
		return sizeof(short) * 2;
	case D3DDECLTYPE_SHORT4N:	// 	4D 부호 있는 짧은 정규화(v[0] / 32767.0, v[1] / 32767.0, v[2] / 32767.0, v[3] / 32767.0)
		return sizeof(short) * 4;
	case D3DDECLTYPE_USHORT2N:  //2D 부호 없는 짧은 정규화(v[0]/65535.0,v[1]/65535.0,0,1)
		return sizeof(short) * 2;
	case D3DDECLTYPE_USHORT4N:  // 4D 부호 없는 짧은 정규화(v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
		return sizeof(short) * 4;
	case D3DDECLTYPE_UDEC3:		// (값, 값, 값, 1)로 확장된 3D 부호 없는 10 10 10 형식
		return sizeof(unsigned int);
	case D3DDECLTYPE_DEC3N:		// 3D 서명된 10 10 10 형식은 정규화되고 (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)로 확장됩니다.
		return sizeof(unsigned int);
	case D3DDECLTYPE_FLOAT16_2: // (value, value, 0, 1)로 확장된 두 개의 16비트 부동 소수점 값
		return sizeof(short) * 2;
	case D3DDECLTYPE_FLOAT16_4:	//4개의 16비트 부동 소수점 값
		return sizeof(short) * 4;

	default:
		assert( false );
		return 0;
	}
}

//프리미티브= 가장기본적으로 대상을 표현하려고 하는거
//정점 버퍼와 인덱스 스트림을 설정한 다음 인덱스된 프리미티브를 그립니다.
//Direct3D의 다양한 상태 설정 함수들을 호출한 후 DrawIndexedPrimitive를 호출합니다.
HRESULT CompressedMesh::Draw()
{
	HRESULT hRes;
	hRes = m_pD3D->SetStreamSource( 0, m_pVB, 0, m_vertexSize );
	if( FAILED(hRes) ) return hRes;
	hRes = m_pD3D->SetIndices( m_pIB );
	if( FAILED(hRes) ) return hRes;
	hRes = m_pD3D->SetVertexDeclaration( m_pDecl );
	if( FAILED(hRes) ) return hRes;

	hRes = m_pD3D->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_numVertex, 0, m_numIndices/3 );
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

//입력 메시의 인덱스 버퍼를 현재 객체의 인덱스 버퍼로 복사합니다.
//16비트 인덱스 버퍼만을 처리합니다.
HRESULT CompressedMesh::CopyIndexBuffer( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	m_numIndices = in->GetNumFaces() * 3;

	const unsigned int ibsize = m_numIndices * sizeof(WORD);
	//인덱스 버퍼 복사(16비트만 해당)
	hRes = m_pD3D->CreateIndexBuffer( ibsize, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_pIB, NULL );
	if( FAILED(hRes) ) return hRes;

	WORD* inIBStream = 0;
	hRes = in->LockIndexBuffer( D3DLOCK_READONLY, (void**)&inIBStream);
	if( FAILED(hRes) ) return hRes;
	WORD* outIBStream = 0;
	hRes = m_pIB->Lock(0,0, (void**)&outIBStream, 0);
	if( FAILED(hRes) ) return hRes;

	memcpy(outIBStream, inIBStream, ibsize);

	// 이제 두 인덱스 버퍼를 모두 잠금 해제합니다.
	hRes = m_pIB->Unlock();
	if( FAILED(hRes) ) return hRes;
	hRes = in->UnlockIndexBuffer();
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

//압축 기능
//3개의 부동 소수점으로 주어진 법선을 양자화하여 D3DCOLOR 형식의 값을 반환합니다.
//법선을 0~255 범위로 정규화합니다.
unsigned int CompressedMesh::QuantiseNormal( const float nx, const float ny, const float nz)
{
	// -1.0 - 1.f -> 0.f - 255.f
	unsigned int ix = unsigned int( (nx * 127.f) + 128.f );
	unsigned int iy = unsigned int( (ny * 127.f) + 128.f );
	unsigned int iz = unsigned int( (nz * 127.f) + 128.f );
	unsigned int iw = 0; // w 구성 요소를 사용하지 않습니다(패딩만 사용).

	// D3DCOLOR 사용으로 인해 구성요소를 사전 교체합니다(인텔 바이트 편의성 사용).
	unsigned int out = (iw << 24) | (ix << 16) | (iy << 8) | (iz << 0);

	return out;
}

//입력 메시의 법선을 양자화하고, 이를 기반으로 새로운 메시를 생성합니다.
//정점 버퍼를 생성하고, 위치와 양자화된 법선을 복사합니다.
HRESULT CompressedMesh::QuantiseNormals( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 함수 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_FLOAT3;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_FLOAT3);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_FLOAT3) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	//위치 복사 + 법선 양자화
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 위치에 대해 3개의 부동소수점을 복사합니다.
		outStream[0] = inStream[0]; // pos.x
		outStream[1] = inStream[1]; // pos.y
		outStream[2] = inStream[2]; // pos.z

		// 정상을 정량화하다
		float nx = inStream[3];
		float ny = inStream[4];
		float nz = inStream[5];
		unsigned int quant = QuantiseNormal( nx, ny, nz );
		((unsigned int*)outStream)[3] = quant;

		//다음 꼭지점
		inStream += 6;
		outStream += 4;
	}
	
	//이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) ) 
		return hRes;

	return S_OK;
}

//위치 벡터를 D3DCOLOR 형식으로 압축하여 반환합니다.
//위치 값을 8비트로 스케일링하고 오프셋을 적용합니다.
unsigned int CompressedMesh::ScaleAndOffsetPosition8bit( const D3DXVECTOR3& in )
{
	D3DXVECTOR3 fv;
	
	fv[0] = (in[0] - m_soOffset[0]) / m_soScale[0];
	fv[1] = (in[1] - m_soOffset[1]) / m_soScale[1];
	fv[2] = (in[2] - m_soOffset[2]) / m_soScale[2];

	unsigned int ix = unsigned int (fv[0] * 255.f);
	unsigned int iy = unsigned int (fv[1] * 255.f);
	unsigned int iz = unsigned int (fv[2] * 255.f);
	unsigned int iw = 0; // w 구성요소를 사용하지 마세요


	// D3DCOLOR 사용으로 인해 구성요소를 사전 교체합니다(인텔 바이트 편의성 사용).
	unsigned int out = (iw << 24) | (ix << 16) | (iy << 8) | (iz << 0);

	return out;
}

// 입력 메시의 위치를 8비트 스케일과 오프셋을 적용한 값으로 변환하고, 새로운 메시를 생성합니다.
// 법선도 양자화하여 포함시킵니다.
HRESULT CompressedMesh::ScaleAndOffsetPosition8bit( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 함수 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	D3DXMATRIX identity;
	D3DXMatrixIdentity( &identity );

	// 스케일과 오프셋 결정
	hRes = DetermineScaleAndOffset(in, identity, m_soScale, m_soOffset );
	if( FAILED(hRes) ) return hRes;

	// 크기 조정 및 오프셋 위치 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );

		unsigned int so = ScaleAndOffsetPosition8bit( pos );
		((unsigned int*)outStream)[0] = so;

		// 법선을 양자화
		((unsigned int*)outStream)[1] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);
		// 다음 정점
		inStream += 6;
		outStream += 2;
	}

	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) ) 
		return hRes;

	return S_OK;
}

//위치 벡터를 16비트 형식으로 압축하여 반환합니다.
//위치 값을 16비트로 스케일링하고 오프셋을 적용합니다.
void CompressedMesh::ScaleAndOffsetPosition16bit( const D3DXVECTOR3& in,  short& x, short& y, short& z, short& w)
{
	D3DXVECTOR3 fv;
	
	fv[0] = (in[0] - m_soOffset[0]) / m_soScale[0];
	fv[1] = (in[1] - m_soOffset[1]) / m_soScale[1];
	fv[2] = (in[2] - m_soOffset[2]) / m_soScale[2];

	x = short( (fv[0]-0.5f) * 65535.f);
	y = short( (fv[1]-0.5f) * 65535.f);
	z = short( (fv[2]-0.5f) * 65535.f);
	w = short(-0.5f * 65535.f); // 0
}

//입력 메시의 위치를 16비트 스케일과 오프셋을 적용한 값으로 변환하고, 새로운 메시를 생성합니다.
//법선도 양자화하여 포함시킵니다.
HRESULT CompressedMesh::ScaleAndOffsetPosition16bit( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 함수 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_SHORT4;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_SHORT4);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_SHORT4) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	D3DXMATRIX identity;
	D3DXMatrixIdentity( &identity );
	// 스케일과 오프셋 결정
	hRes = DetermineScaleAndOffset(in, identity, m_soScale, m_soOffset );
	if( FAILED(hRes) ) return hRes;

	// 크기 조정 및 오프셋 위치 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );

		short x,y,z,w;
		ScaleAndOffsetPosition16bit( pos, x, y, z, w );
		((short*)outStream)[0] = x;
		((short*)outStream)[1] = y;
		((short*)outStream)[2] = z;
		((short*)outStream)[3] = w;

		// 법선을 양자화
		((unsigned int*)outStream)[2] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);

		// 다음 정점
		inStream += 6;
		outStream += 3;
	}
	
	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

//배율 및 오프셋 결정:
//메쉬 정점 위치를 정규화하기 위한 배율 및 오프셋 값을 결정합니다.
HRESULT CompressedMesh::DetermineScaleAndOffset(ID3DXBaseMesh* in, D3DXMATRIX& transform,D3DXVECTOR3& scale, D3DXVECTOR3& offset )
{
	HRESULT hRes;

	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	D3DXVECTOR3 lowerRange, upperRange;
	lowerRange[0] = FLT_MAX;
	lowerRange[1] = FLT_MAX;
	lowerRange[2] = FLT_MAX;
	upperRange[0] = -FLT_MAX;
	upperRange[1] = -FLT_MAX;
	upperRange[2] = -FLT_MAX;

	// 스케일과 오프셋 결정
	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		D3DXVECTOR3 vec(inStream[0], inStream[1], inStream[2]);
		D3DXVec3TransformCoord( &vec, &vec, &transform );

		lowerRange[0] = min(vec[0], lowerRange[0]);
		lowerRange[1] = min(vec[1], lowerRange[1]);
		lowerRange[2] = min(vec[2], lowerRange[2]);

		upperRange[0] = max(vec[0], upperRange[0]);
		upperRange[1] = max(vec[1], upperRange[1]);
		upperRange[2] = max(vec[2], upperRange[2]);

		// 다음 정점
		inStream += 6;
	}

	offset = lowerRange;
	scale = upperRange - lowerRange;

	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	return S_OK;
}

//회전 행렬 결정:
//입력 메시를 변환하기 위한 회전 행렬을 계산합니다.
HRESULT CompressedMesh::DetermineRotationMatrix( ID3DXBaseMesh* in, D3DXMATRIX& matrix)
{
	HRESULT hRes;

	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	// 인덱스 데이터 잠금
	WORD* inIBStream = 0;
	hRes = in->LockIndexBuffer( D3DLOCK_READONLY, (void**)&inIBStream);
	if( FAILED(hRes) ) return hRes;

	double sum[3] = {0,0,0};
	unsigned int n = in->GetNumFaces();

	// 각 삼각형의 위치 데이터를 합산합니다.
	for(unsigned int i=0;i < n;++i)
	{
		unsigned int i0 = inIBStream[(i * 3) + 0];
		unsigned int i1 = inIBStream[(i * 3) + 1];
		unsigned int i2 = inIBStream[(i * 3) + 2];

		sum[0] += inStream[(i0 * 6) + 0];
		sum[1] += inStream[(i0 * 6) + 1];
		sum[2] += inStream[(i0 * 6) + 2];

		sum[0] += inStream[(i1 * 6) + 0];
		sum[1] += inStream[(i1 * 6) + 1];
		sum[2] += inStream[(i1 * 6) + 2];

		sum[0] += inStream[(i2 * 6) + 0];
		sum[1] += inStream[(i2 * 6) + 1];
		sum[2] += inStream[(i2 * 6) + 2];
	}

	// 	계산 u
	double u[3];
	u[0] = (1.f/(3.f * n)) * sum[0];
	u[1] = (1.f/(3.f * n)) * sum[1];
	u[2] = (1.f/(3.f * n)) * sum[2];

	double R[3][3]; 	// 수치 정밀도 문제로 인해 double이 도움이 됩니다.(float 부동 소수점 그만 쓰라고)

	// 회전 행렬 계산
	for(unsigned int i=0; i < n;++i)
	{
		unsigned int i0 = inIBStream[(i * 3) + 0];
		unsigned int i1 = inIBStream[(i * 3) + 1];
		unsigned int i2 = inIBStream[(i * 3) + 2];

		double p[3],q[3],r[3];

		p[0] = inStream[(i0 * 6) + 0] - u[0];
		p[1] = inStream[(i0 * 6) + 1] - u[1];
		p[2] = inStream[(i0 * 6) + 2] - u[2];

		q[0] = inStream[(i1 * 6) + 0] - u[0];
		q[1] = inStream[(i1 * 6) + 1] - u[1];
		q[2] = inStream[(i1 * 6) + 2] - u[2];

		r[0] = inStream[(i2 * 6) + 0] - u[0];
		r[1] = inStream[(i2 * 6) + 1] - u[1];
		r[2] = inStream[(i2 * 6) + 2] - u[2];

		for(unsigned int j = 0; j < 3; j++)
		{
			for(unsigned int k = 0; k < 3; k++)
			{
				R[j][k] =	(p[j]*p[k]) + 
							(q[j]*q[k]) +
							(r[j]*r[k]);
			}
		}
	}

	// 평균 회전 합계
	for(unsigned int j = 0; j < 3; ++j)
	{
		for(unsigned int k = 0; k < 3; ++k)
		{
			R[j][k] = R[j][k] * (1.f/(3.f * n));
		}
	}

	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) return hRes;

	hRes = in->UnlockIndexBuffer();
	if( FAILED(hRes) ) return hRes;

	// 행렬 변환
	double a[10],v[10];
	for(unsigned int j = 0; j < 3; ++j)
	{
		for(unsigned int k = 0; k < 3; ++k)
		{
			a[j * 3 + k+1] = R[j][k];
		}
	}


	// 고유 솔버를 통해 실행
	// 원래는 Numerical Receipes의 jacobi를 사용했지만 그럴 수는 없습니다.
	// 배포되므로 이 버전을 사용합니다.

	// 큐브 행의 수치 정밀도에 세금이 부과되기 때문에 tol과 machineprecision이 너무 높습니다.
	// 큐브 행의 규모에서 '적절한' 작업을 위해 수치 분석을 파헤칠 시간입니다. 
	// 텍스트
	double ld[4], le[4];
	double tol = 1.e-30;
	tri_diag(a,ld,le,v,3,tol);

	double macheps = 1.e-30 ;//1.e-16;
	if( calc_eigenstructure(ld,le,v,3,macheps) == -1)
		return E_FAIL;

	// D3D 매트릭스 채우기
	D3DXMatrixIdentity( &matrix );
	matrix._11 = (float)v[1]; matrix._21 = (float)v[2];	matrix._31 = (float)v[3];
	matrix._12 = (float)v[4]; matrix._22 = (float)v[5];	matrix._32 = (float)v[6];
	matrix._13 = (float)v[7]; matrix._23 = (float)v[8];	matrix._33 = (float)v[9];

	return S_OK;
}

//8비트 압축:
//이 함수는 위치 벡터를 8비트 형식으로 압축합니다. 
//입력 벡터는 행렬( m_ctMatrix)로 변환된 다음 8비트 범위(0-255)에 맞게 크기가 조정됩니다.
//결과 구성 요소는 단일 형식으로 압축 unsigned int됩니다 D3DCOLOR.
//이 방법은 메모리 효율성이 중요하고 약간의 정밀도 손실이 허용되는 애플리케이션에 적합합니다.
unsigned int CompressedMesh::CompressionTransformPosition8bit( const D3DXVECTOR3& in )
{
	D3DXVECTOR3 fv;
	
	D3DXVec3TransformCoord( &fv, &in, &m_ctMatrix );

	unsigned int ix = unsigned int (fv[0] * 255.f);
	unsigned int iy = unsigned int (fv[1] * 255.f);
	unsigned int iz = unsigned int (fv[2] * 255.f);
	unsigned int iw = 255; // 4x4 변환에는 w = 1이 필요합니다.


	// D3DCOLOR 사용으로 인해 구성요소를 사전 교체합니다(인텔 바이트 편의성 사용).
	unsigned int out = (iw << 24) | (ix << 16) | (iy << 8) | (iz << 0);

	return out;
}

//8비트 메쉬 변환 기능:
//이 기능은 전체 메시에 8비트 압축을 적용합니다. 각 정점 위치와 법선을 변환하고 결과를 정점 버퍼에 저장합니다. 
//정점 선언은 D3DCOLOR 유형에 대해 고정 기능 파이프라인과 함께 사용하도록 설정됩니다.
//단계:
//	1. 입력 메시에서 정점 선언을 가져옵니다.
//	2. 압축 형식에 대한 새 정점 선언을 만듭니다.
//	3. 압축된 꼭짓점을 보관할 꼭짓점 버퍼를 만듭니다.
//	4. 결합된 변환 행렬을 사용하여 각 정점 위치를 변환합니다.
//	5. 법선 벡터를 양자화합니다.
//	6. 정점 버퍼의 잠금을 해제하고 인덱스 버퍼를 복사합니다.
// 사용법: 전체 메시를 압축하여 렌더링하는 동안 메모리 사용량을 줄이는 데 사용됩니다.
HRESULT CompressedMesh::CompressionTransformPosition8bit( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 기능 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) 
		return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) )
		return hRes;

	D3DXMATRIX rotMat;
	D3DXMatrixIdentity( &rotMat );

	hRes = DetermineRotationMatrix(in, rotMat);
	if( FAILED(hRes) )
		return hRes;

	D3DXMatrixInverse( &rotMat, 0, &rotMat);

	D3DXVECTOR3 scale, offset;
	hRes = DetermineScaleAndOffset(in, rotMat, scale, offset);
	if( FAILED(hRes) )
		return hRes;

	D3DXMATRIX scaleMat, offsetMat;
	D3DXMatrixScaling( &scaleMat, scale[0], scale[1], scale[2] );
	D3DXMatrixTranslation( &offsetMat, offset[0], offset[1], offset[2] );

	D3DXMatrixInverse( &scaleMat, 0, &scaleMat);
	D3DXMatrixInverse( &offsetMat, 0, &offsetMat);

	// m = r-1 * o-1 * s-1
	D3DXMATRIX combined;
	D3DXMatrixMultiply( &combined, &offsetMat, &scaleMat );
	D3DXMatrixMultiply( &combined, &rotMat, &combined );

	m_ctMatrix = combined;

	// 	위치를 압축 공간으로 변환 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 축척 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );

		unsigned int ct = CompressionTransformPosition8bit( pos );
		((unsigned int*)outStream)[0] = ct;

		// 법선을 수량화하다
		((unsigned int*)outStream)[1] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);

		// 다음 정점
		inStream += 6;
		outStream += 2;
	}
	
	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) )
		return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) )
		return hRes;

	return S_OK;
}

//16비트 압축:
//8비트 방법과 유사하지만 16비트 정밀도를 사용합니다. 
// 변환된 위치 벡터 구성 요소는 크기가 조정되고 short유형으로 변환됩니다.
//8비트 압축보다 더 나은 정밀도를 제공하며 더 정확한 정점 위치가 필요한 애플리케이션에 유용합니다.
void CompressedMesh::CompressionTransformPosition16bit( const D3DXVECTOR3& in,  short& x, short& y, short&z, short& w)
{
	D3DXVECTOR3 fv;
	
	D3DXVec3TransformCoord( &fv, &in, &m_ctMatrix );

	x = short ( (fv[0]-0.5f) * 65535.f);
	y = short ( (fv[1]-0.5f) * 65535.f);
	z = short ( (fv[2]-0.5f) * 65535.f);
	w = 32767;// 4x4 변환에는 w = 1이 필요합니다.
}

//16비트 압축 메쉬 변환 기능:
//8비트 변환과 유사하지만 정점 위치를 저장하는 데 16비트 정밀도를 사용합니다.
//사용법: 8비트 압축에 비해 정점 위치에 대한 더 높은 정밀도가 필요한 경우.
HRESULT CompressedMesh::CompressionTransformPosition16bit( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 함수 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_SHORT4;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_SHORT4);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) 
		return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_SHORT4) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) 
		return hRes;

	D3DXMATRIX rotMat;
	D3DXMatrixIdentity( &rotMat );

	hRes = DetermineRotationMatrix(in, rotMat);
	if( FAILED(hRes) ) 
		return hRes;

	D3DXMatrixInverse( &rotMat, 0, &rotMat);

	D3DXVECTOR3 scale, offset;
	hRes = DetermineScaleAndOffset(in, rotMat, scale, offset);
	if( FAILED(hRes) ) 
		return hRes;

	D3DXMATRIX scaleMat, offsetMat;
	D3DXMatrixScaling( &scaleMat, scale[0], scale[1], scale[2] );
	D3DXMatrixTranslation( &offsetMat, offset[0], offset[1], offset[2] );

	D3DXMatrixInverse( &scaleMat, 0, &scaleMat);
	D3DXMatrixInverse( &offsetMat, 0, &offsetMat);

	// m = r * o-1 * s-1
	D3DXMATRIX combined;
	D3DXMatrixMultiply( &combined, &offsetMat, &scaleMat );
	D3DXMatrixMultiply( &combined, &rotMat, &combined );

	m_ctMatrix = combined;

	// 위치를 압축 공간으로 변환 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) 
		return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) 
		return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );

		short x,y,z,w;
		CompressionTransformPosition16bit( pos, x, y, z, w );
		((short*)outStream)[0] = x;
		((short*)outStream)[1] = y;
		((short*)outStream)[2] = z;
		((short*)outStream)[3] = w;

		// 법선을 양자화
		((unsigned int*)outStream)[2] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);

		// 다음 정점
		inStream += 6;
		outStream += 3;
	}
	
	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) )
		return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) ) 
		return hRes;

	return S_OK;
}

//슬라이딩 16비트 압축:
//이 방법은 24비트 정수를 3개의 8비트 세그먼트로 나누는 슬라이딩 윈도우 압축을 제공합니다. 
//위치 벡터와 법선 벡터를 모두 처리하여 배열로 묶습니다 unsigned int.
//사용법: 압축된 위치와 법선이 모두 필요한 응용 프로그램에 사용됩니다.
void CompressedMesh::SlidingCompressionTransformPosition16bit( const D3DXVECTOR3& pos,  const D3DXVECTOR3& norm, unsigned int* outStream)
{
	D3DXVECTOR3 out;
	D3DXVec3TransformCoord( &out, &pos, &m_ctMatrix );
	unsigned int x,y,z;

	// float는 23비트 정밀도만 갖고 24가 필요하므로 double을 사용!
	double tx,ty,tz;

	tx = double(out.x) * 8388607.0; 
	ty = double(out.y) * 8388607.0; 
	tz = double(out.z) * 8388607.0;

	// 정밀도는 24비트입니다.
	x = unsigned int ( tx );
	y = unsigned int ( ty );
	z = unsigned int ( tz );

	// 위치를 24비트 정수에서 3개의 8비트 정수로 분할합니다.
	unsigned int ilx = (x & 0x0000FF) >> 0;
	unsigned int imx = (x & 0x00FF00) >> 8;
	unsigned int ihx = (x & 0xFF0000) >> 16;
	unsigned int ily = (y & 0x0000FF) >> 0;
	unsigned int imy = (y & 0x00FF00) >> 8;
	unsigned int ihy = (y & 0xFF0000) >> 16;
	unsigned int ilz = (z & 0x0000FF) >> 0;
	unsigned int imz = (z & 0x00FF00) >> 8;
	unsigned int ihz = (z & 0xFF0000) >> 16;

	// 법선 -1.0 - 1.f -> 0.f - 255.f
	unsigned int inx = unsigned int( (norm.x * 127.f) + 128.f );
	unsigned int iny = unsigned int( (norm.y * 127.f) + 128.f );
	unsigned int inz = unsigned int( (norm.z * 127.f) + 128.f );

	// D3DCOLOR 사용으로 인해 구성요소를 사전 교체합니다(인텔 바이트 편의성 사용).
	outStream[0] = (inx << 24) | (ilx << 16) | (ily << 8) | (ilz << 0);
	outStream[1] = (iny << 24) | (imx << 16) | (imy << 8) | (imz << 0);
	outStream[2] = (inz << 24) | (ihx << 16) | (ihy << 8) | (ihz << 0);

}

//슬라이딩 16비트 압축 메쉬 변환 기능:
//전체 메시에 슬라이딩 16비트 압축을 적용하여 위치와 법선을 모두 처리합니다.
//정확한 위치와 법선이 모두 필요한 보다 복잡한 장면에 적합한 정밀도와 메모리 사용량 간의 균형 잡힌 접근 방식을 제공합니다.
HRESULT CompressedMesh::SlidingCompressionTransformPosition16bit( ID3DXBaseMesh* in )
{
	HRESULT hRes;
	
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) 
		return hRes;

	// 선언 D3D 생성
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[1].UsageIndex = 1;

	m_Decl[2].Stream	= 0;
	m_Decl[2].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_Decl[2].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[2].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[2].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[2].UsageIndex = 2;

	// D3DDECL_END()
	m_Decl[3].Stream	= 0xFF;
	m_Decl[3].Offset	= 0;
	m_Decl[3].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[3].Method	= 0;
	m_Decl[3].Usage		= 0;
	m_Decl[3].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) return hRes;

	m_vertexSize =	SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR) + 
					SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR) +
					SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) 
		return hRes;

	D3DXMATRIX rotMat;
	D3DXMatrixIdentity( &rotMat );

	hRes = DetermineRotationMatrix(in, rotMat);
	if( FAILED(hRes) ) 
		return hRes;

	D3DXMatrixInverse( &rotMat, 0, &rotMat);

	D3DXVECTOR3 scale, offset;
	hRes = DetermineScaleAndOffset(in, rotMat, scale, offset);
	if( FAILED(hRes) )
		return hRes;

	D3DXMATRIX scaleMat, offsetMat;
	D3DXMatrixScaling( &scaleMat, scale[0], scale[1], scale[2] );
	D3DXMatrixTranslation( &offsetMat, offset[0], offset[1], offset[2] );

	D3DXMatrixInverse( &scaleMat, 0, &scaleMat);
	D3DXMatrixInverse( &offsetMat, 0, &offsetMat);

	// m = r-1 * o-1 * s-1
	D3DXMATRIX combined;
	D3DXMatrixMultiply( &combined, &offsetMat, &scaleMat );
	D3DXMatrixMultiply( &combined, &rotMat, &combined );

	m_ctMatrix = combined;

	// 위치를 압축 공간으로 변환 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	unsigned int* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );
		D3DXVECTOR3 norm( inStream[3], inStream[4], inStream[5] );

		SlidingCompressionTransformPosition16bit( pos, norm, outStream );


		// 다음 정점
		inStream += 6;
		outStream += 3;
	}
	
	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) )
		return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) )
		return hRes;

	return S_OK;
}

//DEC3N 압축:
//위치 벡터를 DEC3N 형식으로 압축합니다. 이 형식은 데이터를 x, y 및 z 구성 요소 각각에 대해 10비트인 32비트 정수로 저장합니다.
//사용법: 이 형식은 DEC3N 형식을 지원하고 컴팩트한 스토리지로 높은 정밀도가 필요한 GPU에 적합합니다.
unsigned int CompressedMesh::CompressionTransformPositionDEC3N( const D3DXVECTOR3& in )
{
	D3DXVECTOR3 fv;
	
	D3DXVec3TransformCoord( &fv, &in, &m_ctMatrix );

   int x = (int) ((fv[0]-0.5f) * 1023.f);
   int y = (int) ((fv[1]-0.5f) * 1023.f);
   int z = (int) ((fv[2]-0.5f) * 1023.f);

   // 이 2비트는 DEC3N에 의해 ​​폐기됩니다.
   unsigned int lost = 0x1;

   return (x & 0x3FF) | ((y & 0x3FF) << 10) | ((z & 0x3FF) << 20) | ((lost & 0x3) << 30);

}

//DEC3N 압축 메쉬 변환 기능:
//정점 위치를 압축하기 위해 DEC3N 형식을 사용합니다. 
//이 함수는 새로운 정점 선언과 정점 버퍼를 생성한 다음 DEC3N 형식을 사용하여 각 정점을 압축합니다.
//사용법: DEC3N 형식을 지원하고 효율적인 저장과 함께 높은 정밀도가 필요한 하드웨어를 대상으로 하는 경우 가장 적합합니다.
HRESULT CompressedMesh::CompressionTransformPositionDEC3N( ID3DXBaseMesh* in )
{
	HRESULT hRes;

	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration( inDecl );
	if( FAILED(hRes) ) return hRes;

	// 선언 D3D 생성(고정 함수 레지스터)
	m_Decl[0].Stream	= 0;
	m_Decl[0].Offset	= 0;
	m_Decl[0].Type		= D3DDECLTYPE_DEC3N;
	m_Decl[0].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	m_Decl[0].UsageIndex = 0;
	
	m_Decl[1].Stream	= 0;
	m_Decl[1].Offset	= SizeOfD3DVertexType(D3DDECLTYPE_DEC3N);
	m_Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	m_Decl[1].Method	= D3DDECLMETHOD_DEFAULT;
	m_Decl[1].Usage		= D3DDECLUSAGE_NORMAL;
	m_Decl[1].UsageIndex = 0;
	
	// D3DDECL_END()
	m_Decl[2].Stream	= 0xFF;
	m_Decl[2].Offset	= 0;
	m_Decl[2].Type		= D3DDECLTYPE_UNUSED;
	m_Decl[2].Method	= 0;
	m_Decl[2].Usage		= 0;
	m_Decl[2].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration( m_Decl, &m_pDecl );
	if( FAILED(hRes) ) return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_DEC3N) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_DEFAULT, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	D3DXMATRIX rotMat;
	D3DXMatrixIdentity( &rotMat );

	hRes = DetermineRotationMatrix(in, rotMat);
	if( FAILED(hRes) ) return hRes;

	D3DXMatrixInverse( &rotMat, 0, &rotMat);

	D3DXVECTOR3 scale, offset;
	hRes = DetermineScaleAndOffset(in, rotMat, scale, offset);
	if( FAILED(hRes) ) return hRes;

	D3DXMATRIX scaleMat, offsetMat;
	D3DXMatrixScaling( &scaleMat, scale[0], scale[1], scale[2] );
	D3DXMatrixTranslation( &offsetMat, offset[0], offset[1], offset[2] );

	D3DXMatrixInverse( &scaleMat, 0, &scaleMat);
	D3DXMatrixInverse( &offsetMat, 0, &offsetMat);

	// m = r-1 * o-1 * s-1
	D3DXMATRIX combined = scaleMat;
	D3DXMatrixMultiply( &combined, &offsetMat, &combined);
	D3DXMatrixMultiply( &combined, &rotMat, &combined );

	m_ctMatrix = combined;

	// 위치를 압축 공간으로 변환 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int i=0;i < in->GetNumVertices();++ i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos( inStream[0], inStream[1], inStream[2] );

		unsigned int ct = CompressionTransformPositionDEC3N( pos );
		((unsigned int*)outStream)[0] = ct;

		// 법선을 양자화
		((unsigned int*)outStream)[1] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);

		// 다음 정점
		inStream += 6;
		outStream += 2;
	}
	
	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) return hRes;
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) return hRes;

	hRes = CopyIndexBuffer(in);
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

//단일 32비트 부호 없는 정수 압축:
//X와 Y는 각각 10비트에 맞게 크기가 조정됩니다.
//Z는 12비트에 맞게 조정됩니다.
//X는 최하위 10비트에 저장됩니다.
//Y는 다음 10비트에 저장됩니다.
//Z는 다음 12비트에 저장됩니다.
unsigned int CompressedMesh::CompressionTransformPosition101012( const D3DXVECTOR3& in )
{
	D3DXVECTOR3 fv;
	
	D3DXVec3TransformCoord( &fv, &in, &m_ctMatrix );

	int x = (int) (fv[0] * 1023.f); // 10 bits
	int y = (int) (fv[1] * 1023.f); // 10 bits
	int z = (int) (fv[2] * 4095.f); // 12 bits
	
	return (x & 0x3FF) | ((y & 0x3FF) << 10) | ((z & 0xFFF) << 20);
}

//축을 교환하는 D3DXMATRIX를 생성합니다.
void CompressedMesh::CreateSwapMatrix( D3DXMATRIX& in, SW_AXIS xAxis, SW_AXIS yAxis, SW_AXIS zAxis )
{
	D3DXMatrixIdentity( &in );

	switch( xAxis )
	{
	case SWA_X:
		in._11 = 1.f;
		in._21 = 0.f;
		in._31 = 0.f;
		break;
	case SWA_Y:
		in._11 = 0.f;
		in._21 = 1.f;
		in._31 = 0.f;
		break;
	case SWA_Z:
		in._11 = 0.f;
		in._21 = 0.f;
		in._31 = 1.f;
		break;
	}

	switch( yAxis )
	{
	case SWA_X:
		in._12 = 1.f;
		in._22 = 0.f;
		in._32 = 0.f;
		break;
	case SWA_Y:
		in._12 = 0.f;
		in._22 = 1.f;
		in._32 = 0.f;
		break;
	case SWA_Z:
		in._12 = 0.f;
		in._22 = 0.f;
		in._32 = 1.f;
		break;
	}

	switch( zAxis )
	{
	case SWA_X:
		in._13 = 1.f;
		in._23 = 0.f;
		in._33 = 0.f;
		break;
	case SWA_Y:
		in._13 = 0.f;
		in._23 = 1.f;
		in._33 = 0.f;
		break;
	case SWA_Z:
		in._13 = 0.f;
		in._23 = 0.f;
		in._33 = 1.f;
		break;
	}

}

//단일 32비트 부호 없는 정수 압축 메쉬 변환:
HRESULT CompressedMesh::CompressionTransformPosition101012(ID3DXBaseMesh* in)
{
	HRESULT hRes;
	// 입력 메시의 스트림 선언을 검색합니다.
	D3DVERTEXELEMENT9 inDecl[MAX_FVF_DECL_SIZE];
	hRes = in->GetDeclaration(inDecl);
	if (FAILED(hRes))
		return hRes;

	// 선언 D3D 생성
	int curDeclNum = 0;

	// 위치의 10,10,10 부분
	m_Decl[curDeclNum].Stream = 0;
	m_Decl[curDeclNum].Offset = 0;
	m_Decl[curDeclNum].Type = D3DDECLTYPE_UDEC3;
	m_Decl[curDeclNum].Method = D3DDECLMETHOD_DEFAULT;
	m_Decl[curDeclNum].Usage = D3DDECLUSAGE_POSITION;
	m_Decl[curDeclNum].UsageIndex = 0;

	curDeclNum++;

	// 위치 낭비 2비트(스트림 오프셋으로 인해)
	m_Decl[curDeclNum].Stream = 0;
	m_Decl[curDeclNum].Offset = 0;
	m_Decl[curDeclNum].Type = D3DDECLTYPE_UBYTE4;
	m_Decl[curDeclNum].Method = D3DDECLMETHOD_DEFAULT;
	m_Decl[curDeclNum].Usage = D3DDECLUSAGE_POSITION;
	m_Decl[curDeclNum].UsageIndex = 1;

	curDeclNum++;

	// 정상
	m_Decl[curDeclNum].Stream = 0;
	m_Decl[curDeclNum].Offset = SizeOfD3DVertexType(D3DDECLTYPE_UDEC3);
	m_Decl[curDeclNum].Type = D3DDECLTYPE_D3DCOLOR;
	m_Decl[curDeclNum].Method = D3DDECLMETHOD_DEFAULT;
	m_Decl[curDeclNum].Usage = D3DDECLUSAGE_NORMAL;
	m_Decl[curDeclNum].UsageIndex = 0;

	curDeclNum++;

	// D3DDECL_END()
	m_Decl[curDeclNum].Stream = 0xFF;
	m_Decl[curDeclNum].Offset = 0;
	m_Decl[curDeclNum].Type = D3DDECLTYPE_UNUSED;
	m_Decl[curDeclNum].Method = 0;
	m_Decl[curDeclNum].Usage = 0;
	m_Decl[curDeclNum].UsageIndex = 0;

	hRes = m_pD3D->CreateVertexDeclaration(m_Decl, &m_pDecl);
	if (FAILED(hRes))
		return hRes;

	m_vertexSize = SizeOfD3DVertexType(D3DDECLTYPE_DEC3N) + SizeOfD3DVertexType(D3DDECLTYPE_D3DCOLOR);
	m_numVertex = in->GetNumVertices();
	const unsigned int vbSize = m_vertexSize * m_numVertex;

	hRes = m_pD3D->CreateVertexBuffer(vbSize, 0, 0, D3DPOOL_DEFAULT, &m_pVB, NULL);
	if (FAILED(hRes))
		return hRes;

	D3DXMATRIX rotMat;
	D3DXMatrixIdentity(&rotMat);

	hRes = DetermineRotationMatrix(in, rotMat);
	if (FAILED(hRes))
		return hRes;

	D3DXMatrixInverse(&rotMat, 0, &rotMat);

	D3DXVECTOR3 scale, offset;
	hRes = DetermineScaleAndOffset(in, rotMat, scale, offset);
	if (FAILED(hRes))
		return hRes;

	D3DXMATRIX scaleMat, offsetMat;
	D3DXMatrixScaling(&scaleMat, scale[0], scale[1], scale[2]);
	D3DXMatrixTranslation(&offsetMat, offset[0], offset[1], offset[2]);

	SW_AXIS minorAxis = SWA_X;
	SW_AXIS otherAxis = SWA_Y;
	SW_AXIS majorAxis = SWA_Z;

	//장축과 단축을 찾습니다.
	if (scale[0] > scale[1])
	{
		if (scale[0] > scale[2])
		{
			majorAxis = SWA_X;
			if (scale[1] > scale[2])
				minorAxis = SWA_Z;
			else
				minorAxis = SWA_Y;
		}
		else
		{
			majorAxis = SWA_Z;
			minorAxis = SWA_Y;
		}
	}
	else
	{
		if (scale[1] > scale[2])
		{
			majorAxis = SWA_Y;
			if (scale[0] > scale[2])
				minorAxis = SWA_Z;
			else
				minorAxis = SWA_X;
		}
		else
		{
			majorAxis = SWA_Z;
			minorAxis = SWA_X;
		}
	}

	switch (majorAxis)
	{
	case SWA_X:
		switch (minorAxis)
		{
		case SWA_Y:
			otherAxis = SWA_Z;
			break;
		case SWA_Z:
			otherAxis = SWA_Y;
			break;
		}
		break;
	case SWA_Y:
		switch (minorAxis)
		{
		case SWA_X:
			otherAxis = SWA_Z;
			break;
		case SWA_Z:
			otherAxis = SWA_X;
			break;
		}
		break;
	case SWA_Z:
		switch (minorAxis)
		{
		case SWA_X:
			otherAxis = SWA_Y;
			break;
		case SWA_Y:
			otherAxis = SWA_X;
			break;
		}
		break;
	}

	// 주요 구성 요소를 Z에 넣는 스왑 행렬을 만듭니다.
	D3DXMATRIX swapMat;
	CreateSwapMatrix(swapMat, minorAxis, otherAxis, majorAxis);

	D3DXMatrixInverse(&scaleMat, 0, &scaleMat);
	D3DXMatrixInverse(&offsetMat, 0, &offsetMat);
	D3DXMatrixInverse(&swapMat, 0, &swapMat);

	// m = r-1 * o-1 * s-1 * sw-1
	D3DXMATRIX combined = swapMat;
	D3DXMatrixMultiply(&combined, &scaleMat, &combined);
	D3DXMatrixMultiply(&combined, &offsetMat, &combined);
	D3DXMatrixMultiply(&combined, &rotMat, &combined);

	m_ctMatrix = combined;

	// 위치를 압축 공간으로 변환 + 법선 복사
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if (FAILED(hRes))
		return hRes;

	float* outStream = 0;
	hRes = m_pVB->Lock(0, 0, (void**)&outStream, 0);
	if (FAILED(hRes))
		return hRes;

	for (unsigned int i = 0; i < in->GetNumVertices(); ++i)
	{
		// 크기 및 오프셋 위치
		D3DXVECTOR3 pos(inStream[0], inStream[1], inStream[2]);

		unsigned int ct = CompressionTransformPosition101012(pos);
		((unsigned int*)outStream)[0] = ct;

		// 법선을 양자화
		((unsigned int*)outStream)[1] = QuantiseNormal(inStream[3], inStream[4], inStream[5]);

		// 다음 정점
		inStream += 6;
		outStream += 2;
	}

	// 이제 두 정점 버퍼를 모두 잠금 해제합니다.
	hRes = m_pVB->Unlock();
	if (FAILED(hRes)) return hRes;
	hRes = in->UnlockVertexBuffer();
	if (FAILED(hRes)) return hRes;

	hRes = CopyIndexBuffer(in);
	if (FAILED(hRes)) return hRes;

	return S_OK;
}

/*-------------------------------------------
공부 내용:
메시를 압축하는 이유는 다음과 같습니다:

1. 메모리 효율성: 원본 메시의 데이터를 압축하여 메모리 사용량을 줄일 수 있습니다. 특히 대규모 모델의 경우, 메모리 효율성은 중요합니다.
2. 데이터 전송 최적화: 압축된 메시는 전송 시간을 단축하고 네트워크 대역폭을 절약할 수 있습니다. 이는 온라인 멀티플레이어 게임 또는 원격 데이터 공유 시 유용합니다.
3. 렌더링 성능 향상: 압축된 메시는 GPU에서 처리하기 쉽습니다. 이로 인해 렌더링 속도가 향상될 수 있습니다.
4. 보안: 압축된 메시는 원본 데이터를 가리키는 데 사용되는 메모리와 디스크 공간을 숨길 수 있습니다. 이는 게임 또는 3D 애플리케이션에서 지적 재산권 보호에 중요할 수 있습니다.
5. 파일 크기 감소: 압축된 메시는 저장할 때 더 적은 공간을 차지합니다. 따라서 디스크 공간을 절약할 수 있습니다.
6. 데이터 보존: 압축된 형식으로 메시를 저장하면 데이터의 무결성을 보존할 수 있습니다. 이는 데이터 전송 또는 장기간 저장 시 중요합니다.
---------------------------------------------*/