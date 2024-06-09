// 다양한 삼각형의 내부 점을 계산하기 위한 HLSL 코드입니다.

//큐브 하나를 구성하는 삼각형의 개수입니다. 
//큐브는 6개의 면으로 구성되며, 각 면은 2개의 삼각형으로 구성되므로 총 12개의 삼각형이 있습니다.
#define NUM_BASE_TRIANGLE 12

//각 삼각형의 정점 위치를 저장하는 배열입니다. 
//각 삼각형은 3개의 정점으로 구성되므로 총 36개의 정점 위치를 저장합니다.
vector VertexPos[3 * NUM_BASE_TRIANGLE];
matrix MVP; //모델-뷰-프로젝션 행렬입니다. 이는 월드 좌표를 클립 좌표로 변환하는 데 사용됩니다.

struct VS_OUT
{
	float4 pos : POSITION;
	float4 col : COLOR;
};

VS_OUT mainVS( float3 vertexStream : POSITION0  ) 
{
	VS_OUT outp;
	//vertexStream의 x, y 값은 바리센트릭 좌표(barycentric coordinates)이며, z 값은 삼각형 인덱스의 일부입니다.
	float i =  vertexStream.x;
	float j =  vertexStream.y;
	//삼각형의 각 꼭짓점에 대한 가중치를 나타냅니다. 이 값들을 사용하여 삼각형 내부의 임의의 점을 계산합니다.
	float k = 1.0 - i - j;
	float baseIndex =  vertexStream.z * 256;
	//pos는 바리센트릭 좌표를 사용하여 삼각형 내부의 점을 계산합니다.
	float3 pos =	i * VertexPos[ (baseIndex * 3) + 0 ] + 
					j * VertexPos[ (baseIndex * 3) + 1 ] + 
					k * VertexPos[ (baseIndex * 3) + 2 ];

	//outp.pos는 모델-뷰-프로젝션 행렬을 사용하여 변환된 좌표입니다.
	outp.pos =  mul( float4(pos,1), MVP );
	//outp.col는 바리센트릭 좌표를 색상 값으로 설정합니다.
	outp.col = vector(i,j,k,1);

	return outp;
}

technique T0
{
	pass P0
	{ 
		VertexShader = compile vs_1_1 mainVS();
	}
}

//각 삼각형의 내부 점을 계산하고, 해당 점을 월드 좌표로 변환하여 렌더링합니다.
//바리센트릭 좌표를 사용하여 삼각형 내부의 점을 계산하며, 이 좌표를 색상 값으로도 사용합니다. 
//이 셰이더는 주로 큐브의 내부 점을 시각화하는 데 사용됩니다.
