#if !defined(COMPRESSED_MESH_H_021201_DC)
#define COMPRESSED_MESH_H_021201_DC

//정점 압축
class CompressedMesh
{
protected:
	LPDIRECT3DDEVICE9				m_pD3D;		//d3d 장치에 대한 포인터
	LPDIRECT3DVERTEXBUFFER9			m_pVB;		//버텍스 버퍼 인터페이스에 대한 포인터
	LPDIRECT3DINDEXBUFFER9			m_pIB;		//인덱스 버퍼 인터페이스에 대한 포인터
	D3DVERTEXELEMENT9				m_Decl[MAX_FVF_DECL_SIZE];	// 스트림 선언
	LPDIRECT3DVERTEXDECLARATION9	m_pDecl;		//인덱스 버퍼 인터페이스에 대한 포인터
	unsigned int				m_vertexSize;	// 단일 정점의 크기
	unsigned int				m_numVertex;	// 정점 개수
	unsigned int				m_numIndices;	// 인덱스 개수

	HRESULT			CopyIndexBuffer(ID3DXBaseMesh* in);
	HRESULT			DetermineScaleAndOffset(ID3DXBaseMesh* in, D3DXMATRIX& transform, D3DXVECTOR3& scale, D3DXVECTOR3& offset);
	HRESULT			DetermineRotationMatrix(ID3DXBaseMesh* in, D3DXMATRIX& matrix);
	unsigned int	SizeOfD3DVertexType(const unsigned int type);

	unsigned int				QuantiseNormal(const float nx, const float ny, const float nz);

	D3DXVECTOR3					m_soOffset;		// 스케일 오프셋 방법 Offset
	D3DXVECTOR3					m_soScale;		// 스케일 오프셋 방법 스케일
	unsigned int				ScaleAndOffsetPosition8bit(const D3DXVECTOR3& in);
	void						ScaleAndOffsetPosition16bit(const D3DXVECTOR3& in, short& x, short& y, short& z, short& w);

	D3DXMATRIX					m_ctMatrix;		// 압축 변환 행렬
	unsigned int				CompressionTransformPosition8bit(const D3DXVECTOR3& in);
	void						CompressionTransformPosition16bit(const D3DXVECTOR3& in,short& x, short& y, short& z, short& w);
	void						SlidingCompressionTransformPosition16bit(const D3DXVECTOR3& pos, const D3DXVECTOR3& norm, unsigned int* outStream);

	enum SW_AXIS
	{
		SWA_X,
		SWA_Y,
		SWA_Z
	};

	void CreateSwapMatrix(D3DXMATRIX& in, SW_AXIS xAxis, SW_AXIS yAxis, SW_AXIS zAxis);

	unsigned int CompressionTransformPositionDEC3N(const D3DXVECTOR3& in);
	unsigned int CompressionTransformPosition101012(const D3DXVECTOR3& in);

public:
	CompressedMesh(LPDIRECT3DDEVICE9 pD3D);	// 기본 ctor
	~CompressedMesh();							// dtor

	// D3XMesh를 가져와 법선을 이 압축된 메시로 양자화합니다.
	HRESULT QuantiseNormals(ID3DXBaseMesh* in);

	// D3XMesh를 가져와 이 압축된 메시에 스케일 및 오프셋(8비트)을 적용합니다.
	HRESULT ScaleAndOffsetPosition8bit(ID3DXBaseMesh* in);
	HRESULT ScaleAndOffsetPosition16bit(ID3DXBaseMesh* in);

	HRESULT CompressionTransformPosition8bit(ID3DXBaseMesh* in);
	HRESULT CompressionTransformPosition16bit(ID3DXBaseMesh* in);
	HRESULT SlidingCompressionTransformPosition16bit(ID3DXBaseMesh* in);

	// Dx9의 새로운 것
	HRESULT CompressionTransformPositionDEC3N(ID3DXBaseMesh* in);
	HRESULT CompressionTransformPosition101012(ID3DXBaseMesh* in);

	HRESULT Draw();	// D3XMesh의 하위 집합을 그리는 것과 동일합니다.

	LPDIRECT3DVERTEXBUFFER9	GetVertexBuffer() { return m_pVB; };
	LPDIRECT3DINDEXBUFFER9	GetIndexBuffer() { return m_pIB; };
	D3DVERTEXELEMENT9* GetStreamDeclaration() { return m_Decl; };
	unsigned int			GetVertexSize() const { return m_vertexSize; };

	D3DXVECTOR3& GetScaleOffsetScale() { return m_soScale; };
	D3DXVECTOR3& GetScaleOffsetOffset() { return m_soOffset; };
	D3DXMATRIX& GetCompressionTransfromMatrix() { return m_ctMatrix; };
};
#endif