////------------------
////쉐이더 모델 3
////1.입력 및 출력 구조체 확장
////2.텍스처 매핑 추가
////3.조명 계산 추가
////------------------
//
////추가된 기능 설명
////1.입력 및 출력 구조체 확장
////  - VS_IN 구조체에 texcoord 필드를 추가하여 텍스처 좌표를 전달합니다.
////  - VS_OUT 구조체에 texcoord, norm, worldPos 필드를 추가하여 텍스처 좌표, 정점의 법선, 월드 좌표를 전달합니다.
//// 
////2.텍스처 매핑 추가
////  - TextureSampler 샘플러를 정의하여 텍스처 샘플링을 추가합니다.
////  - 텍스처 좌표를 VS_OUT 구조체로 전달하고, 픽셀 셰이더에서 텍스처 색상을 샘플링합니다.
////
////3.조명 계산 추가
////  - 디렉셔널 라이트 방향(LightDirection), 색상(LightColor), 앰비언트 색상(AmbientColor)을 추가합니다.
////  - 픽셀 셰이더에서 조명 계산을 수행합니다.노말 벡터와 라이트 방향의 내적(dot product)을 계산하여 확산 조명(diffuse lighting)을 적용합니다.
////  - 최종 색상은 텍스처 색상과 조명 색상의 합으로 계산됩니다.
////
////4.그라데이션 추가
////  - gradientFactor는 월드 좌표의 Y 위치를 기반으로 계산됩니다. 여기서 Y 좌표를 10으로 나누어 정규화하고, saturate 함수를 사용하여 0과 1 사이로 클램핑합니다. 필요에 따라 그라데이션 범위를 조정할 수 있습니다.
////  - gradientColor는 lerp 함수를 사용하여 빨강 (1, 0, 0, 1)과 파랑 (0, 0, 1, 1) 사이를 보간합니다.
////  - 최종 색상(finalColor)에 그라데이션 색상(gradientColor)을 곱하여 출력 색상(outp.col)을 계산합니다.
//
//matrix MVP;
//matrix Model;
//matrix View;
//matrix Projection;
//
//#define NUM_CONSTS 20
//#define NUM_BASE_VERTICES (NUM_CONSTS-4) / 2
//#define MAX_DISPLACEMENT_HEIGHT  1
//
//vector VertexPos[NUM_BASE_VERTICES];
//vector VertexNorm[NUM_BASE_VERTICES];
//
//struct VS_IN
//{
//    float2 barycentric : POSITION0;
//    float4 indices_disp : POSITION1;
//    float2 texcoord : TEXCOORD0;
//};
//
//struct VS_OUT
//{
//    float4 pos : POSITION;
//    float4 col : COLOR;
//    float2 texcoord : TEXCOORD0;
//    float3 norm : TEXCOORD1;
//    float3 worldPos : TEXCOORD2;
//};
//
//VS_OUT mainVS(VS_IN vertexStream)
//{
//    VS_OUT outp;
//
//    float i = vertexStream.barycentric.x;
//    float j = vertexStream.barycentric.y;
//    float k = 1.f - i - j;
//    float i0 = vertexStream.indices_disp.x * 256;
//    float i1 = vertexStream.indices_disp.y * 256;
//    float i2 = vertexStream.indices_disp.z * 256;
//    float displace = vertexStream.indices_disp.w * MAX_DISPLACEMENT_HEIGHT;
//
//    // 이 삼각형의 위치와 법선
//    float3 v0 = VertexPos[i0];
//    float3 n0 = VertexNorm[i0];
//    float3 v1 = VertexPos[i1];
//    float3 n1 = VertexNorm[i1];
//    float3 v2 = VertexPos[i2];
//    float3 n2 = VertexNorm[i2];
//    float3 pos, norm;
//
//    // 선형 위치
//    pos = (i * v0) + (j * v1) + (k * v2);
//
//    // 선형 법선
//    norm = (i * n0) + (j * n1) + (k * n2);
//    norm = normalize(norm);
//
//    // 법선을 따라 변위
//    pos += norm * displace;
//
//    // 위치 변환
//    float4 worldPos = float4(pos, 1);
//    outp.pos = mul(worldPos, MVP);
//    outp.worldPos = pos;
//
//    // texcoord를 통과합니다.
//    outp.texcoord = vertexStream.texcoord;
//
//    // 일반 통과
//    outp.norm = norm;
//
//    // 색상 계산
//    outp.col = float4((norm * 0.5) + 0.5, 1);
//
//    return outp;
//}
//
//struct PS_IN
//{
//    float4 pos : POSITION;
//    float4 col : COLOR;
//    float2 texcoord : TEXCOORD0;
//    float3 norm : TEXCOORD1;
//    float3 worldPos : TEXCOORD2;
//};
//
//struct PS_OUT
//{
//    float4 col : COLOR;
//};
//
//sampler2D TextureSampler : register(s0);
//float3 LightDirection;
//float4 LightColor;
//float4 AmbientColor;
//
//PS_OUT mainPS(PS_IN frag)
//{
//    PS_OUT outp;
//
//    // 텍스처 샘플링
//    float4 texColor = tex2D(TextureSampler, frag.texcoord);
//
//    // 조명 계산
//    float3 normal = normalize(frag.norm);
//    float3 lightDir = normalize(LightDirection);
//    float NdotL = max(dot(normal, lightDir), 0.0);
//
//    float4 diffuse = LightColor * NdotL;
//    float4 ambient = AmbientColor;
//
//    // 텍스처 색상과 조명 결합
//    float4 finalColor = texColor * (diffuse + ambient);
//
//    // 월드 Y 위치에 따른 빨간색-파란색 그라데이션
//    float gradientFactor = saturate(frag.worldPos.y / 10.0); // 그래디언트 범위를 제어하기 위해 제수를 조정합니다.
//    float4 gradientColor = lerp(float4(1, 0, 0, 1), float4(0, 0, 1, 1), gradientFactor);
//
//    // 최종 색상을 그라디언트와 결합
//    outp.col = finalColor * frag.col * gradientColor;
//
//    return outp;
//}
//
//technique T0
//{
//    pass P0
//    {
//        VertexShader = compile vs_3_0 mainVS();
//        PixelShader = compile ps_3_0 mainPS();
//    }
//}


//------------------
//쉐이더 모델 1.1 
//------------------

//간단한 변환 및 고정 색상을 위한 HLSL 코드입니다.
matrix MVP;
//MVP는 월드, 뷰, 프로젝션 매트릭스의 결합으로, 변환을 위해 사용됩니다.

// 정점당 벡터 2개 - 행렬의 경우 4개
// 그래서 각각. 우리는 이것을 D3DXMACRO를 통해 코드에서 다음과 같이 정의합니다.
//NUM_CONSTS는 사용 가능한 상수의 총 개수를 정의합니다.
//NUM_BASE_VERTICES는 사용할 기본 정점의 수를 정의합니다.
//MAX_DISPLACEMENT_HEIGHT는 정점 변위의 최대 높이를 정의합니다.
//VertexPos와 VertexNorm은 각각 정점의 위치와 법선을 저장하는 배열입니다.
#define NUM_CONSTS 20
#define NUM_BASE_VERTICES (NUM_CONSTS-4 ) / 2
#define MAX_DISPLACEMENT_HEIGHT  1

vector VertexPos[NUM_BASE_VERTICES];
vector VertexNorm[NUM_BASE_VERTICES];

//VS_IN은 정점 셰이더의 입력 구조체로, 바리센터 좌표(barycentric)와 인덱스 및 변위(indices_disp)를 포함합니다.
//VS_OUT은 정점 셰이더의 출력 구조체로, 변환된 위치(pos)와 색상(col)을 포함합니다.
struct VS_IN
{
    float2 barycentric	: POSITION0;
    float4 indices_disp	: POSITION1;
};

struct VS_OUT
{
    float4 pos : POSITION;
    float4 col : COLOR;
};


//바리센터 좌표(i, j, k)를 사용하여 위치와 법선을 선형 보간합니다.
//변위 값을 법선 방향으로 적용하여 변위된 위치를 계산합니다.
//변위된 위치를 MVP 매트릭스를 사용해 변환합니다.
//법선 벡터를 이용해 색상을 계산합니다.
VS_OUT mainVS(VS_IN vertexStream)
{
    VS_OUT outp;

    float i = vertexStream.barycentric.x;
    float j = vertexStream.barycentric.y;
    float k = 1.f - i - j;
    float i0 = vertexStream.indices_disp.x * 256;
    float i1 = vertexStream.indices_disp.y * 256;
    float i2 = vertexStream.indices_disp.z * 256;
    float displace = vertexStream.indices_disp.w * MAX_DISPLACEMENT_HEIGHT;

    // 이 삼각형의 위치와 법선
    float3 v0 = VertexPos[i0];
    float3 n0 = VertexNorm[i0];
    float3 v1 = VertexPos[i1];
    float3 n1 = VertexNorm[i1];
    float3 v2 = VertexPos[i2];
    float3 n2 = VertexNorm[i2];
    float3 pos, norm;

    // 선형 위치
    pos = (i * v0) + (j * v1) + (k * v2);

    // 선형 법선
    norm = (i * v0) + (j * v1) + (k * v2);
    norm = normalize(norm);

    // 법선을 따라 변위
    pos += norm * displace;

    // 위치 변환
    outp.pos = mul(float4(pos, 1), MVP);
    outp.col = float4((norm * 0.5) + 0.5, 1);
    //	outp.col = displace;

    return outp;
}

//T0 기법과 P0 패스를 정의합니다.
//P0 패스에서는 mainVS 정점 셰이더를 컴파일하여 사용합니다.
technique T0
{
    pass P0
    {
        VertexShader = compile vs_1_1 mainVS();
    }
}
