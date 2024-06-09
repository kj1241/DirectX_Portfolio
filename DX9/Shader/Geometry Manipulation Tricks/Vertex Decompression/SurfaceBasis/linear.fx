////간단한 변환 및 고정 색상을 위한 HLSL 코드입니다.
////셰이더 코드는 큐브의 정점과 법선을 사용하여 변환된 좌표와 색상을 계산하는 작업을 수행합니다.
////이 코드는 정점 셰이더를 정의하고, 모델 - 뷰 - 프로젝션(MVP) 행렬을 사용하여 입력 정점의 위치를 변환하고, 법선을 이용하여 색상을 계산합니다.
//matrix MVP;
//
//// 큐브는 8개의 정점으로 구성되므로, 큐브의 정점 개수를 정의합니다.
//#define NUM_BASE_VERTICES 8
//// 큐브의 각 정점 위치를 저장하는 배열입니다.
//vector VertexPos[NUM_BASE_VERTICES];
//// 큐브의 각 정점 법선 벡터를 저장하는 배열입니다.
//vector VertexNorm[NUM_BASE_VERTICES];
//
//struct VS_IN
//{
//	float2 barycentric	: POSITION0; //barycentric: 바리센트릭 좌표를 나타냅니다.
//	float3 indices		: POSITION1; //indices: 삼각형의 각 정점의 인덱스를 나타냅니다.
//};
//
//struct VS_OUT
//{
//	float4 pos : POSITION; //pos: 변환된 정점 위치로, POSITION 세맨틱을 사용하여 다음 단계로 전달됩니다.
//	float4 col : COLOR; //col: 정점의 색상으로, COLOR 세맨틱을 사용하여 다음 단계로 전달됩니다.
//};
//
//VS_OUT mainVS( VS_IN vertexStream ) 
//{
//	VS_OUT outp;
//	// i, j, k는 바리센트릭 좌표로, 삼각형의 각 꼭짓점에 대한 가중치를 나타냅니다.
//	float i =  vertexStream.barycentric.x;
//	float j =  vertexStream.barycentric.y;
//	float k = 1.f - i - j;
//
//	//i0, i1, i2는 각 정점 인덱스
//	float i0 =  vertexStream.indices.x * 256;
//	float i1 =  vertexStream.indices.y * 256;
//	float i2 =  vertexStream.indices.z * 256;
//
//	//각 정점의 위치(v0, v1, v2)와 법선(n0, n1, n2)을 배열에서 가져옵니다.
//	float3 v0 =	VertexPos[ i0 ];
//	float3 n0 = VertexNorm[ i0 ];
//
//	float3 v1 = VertexPos[ i1 ];
//	float3 n1 = VertexNorm[ i1 ];
//
//	float3 v2 = VertexPos[ i2 ];
//	float3 n2 = VertexNorm[ i2 ];
//
//	float3 pos, norm;
//	// 선형 위치
//	pos = (i * v0) + (j * v1) + (k * v2);
//	// 선혁 백터 
//	norm = (i * v0) + (j * v1) + (k * v2);
//	norm = normalize( norm );
//	
//	// 이것은 단지 정상임을 데모하기 위한 것입니다. 
//	// 매끄럽고 변위 매핑이 수행되지 않습니다.
//	// 균열을 발생시킵니다. 본질적으로 우리는 변위를 가지고 있습니다
//	// 모든 곳에서 0.3의 맵
//	pos += norm * 0.3f;
//
//	// 모든 정점에 대해 변위 맵의 값 0.3을 적용하여 법선 방향으로 이동시킵니다.
//	outp.pos =  mul( float4(pos,1), MVP );
//	outp.col = float4( (norm*0.5)+0.5,1);
//
//	return outp;
//}
//
//technique T0
//{
//	pass P0
//	{ 
//		VertexShader = compile vs_1_1 mainVS();
//	}
//}
//
////환과 법선 기반 색상 계산에 초점을 맞추고 설정되어 있습니다.


//----------------
// 개선 코드 쉐이더 모델 - 3.0
//----------------

//1. 매직 넘버 제거
//현재 코드에서 256 같은 매직 넘버가 사용되고 있습니다.
//이 값들을 상수 또는 매크로로 정의하여 코드의 가독성을 높이고 유지보수를 쉽게 할 수 있습니다.

//2. 중복 코드 제거
//현재 구조체 VS_IN과 VS_OUT은 단순하지만, 필요한 경우 추가적인 데이터를 포함하도록 확장할 수 있습니다. 
//예를 들어, UV 좌표 또는 추가적인 정점 속성을 포함할 수 있습니다.

//3. 쉐이더 프로파일 업그레이드
//현재 셰이더는 vs_1_1 프로파일을 사용하고 있습니다. 
//가능하다면, 더 최신의 셰이더 모델(vs_3_0 이상)로 업그레이드하여 더 많은 기능과 최적화를 사용할 수 있습니다.

// 매트릭스 정의
matrix MVP;

// 큐브 하나에 충분합니다 (8개의 정점)
#define NUM_BASE_VERTICES 8
#define INDEX_SCALE 256

vector VertexPos[NUM_BASE_VERTICES];
vector VertexNorm[NUM_BASE_VERTICES];

struct VS_IN
{
    float2 barycentric : POSITION0;
    float3 indices     : POSITION1;
};

struct VS_OUT
{
    float4 pos : POSITION;
    float4 col : COLOR;
};

VS_OUT mainVS(VS_IN vertexStream)
{
    VS_OUT outp;

    // 바리센트릭 좌표 계산
    float i = vertexStream.barycentric.x;
    float j = vertexStream.barycentric.y;
    float k = 1.f - i - j;
    int index0 = vertexStream.indices.x * INDEX_SCALE;
    int index1 = vertexStream.indices.y * INDEX_SCALE;
    int index2 = vertexStream.indices.z * INDEX_SCALE;

    // 삼각형의 각 정점의 위치 및 법선 벡터 계산
    float3 v0 = VertexPos[index0];
    float3 n0 = VertexNorm[index0];
    float3 v1 = VertexPos[index1];
    float3 n1 = VertexNorm[index1];
    float3 v2 = VertexPos[index2];
    float3 n2 = VertexNorm[index2];

    // 선형 보간으로 위치 및 법선 벡터 계산
    float3 pos = (i * v0) + (j * v1) + (k * v2);
    float3 norm = normalize((i * n0) + (j * n1) + (k * n2));

    // 변위 맵 적용
    pos += norm * 0.3f;

    // 변환된 위치 계산
    outp.pos = mul(float4(pos, 1), MVP);
    // 법선 벡터를 기반으로 색상 계산
    outp.col = float4((norm * 0.5f) + 0.5f, 1);

    return outp;
}

technique T0
{
    pass P0
    {
        VertexShader = compile vs_3_0 mainVS();
    }
}
