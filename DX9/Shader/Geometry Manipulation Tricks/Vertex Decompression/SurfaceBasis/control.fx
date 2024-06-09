//간단한 변환 및 고정 색상을 위한 HLSL 코드입니다.
//셰이더 코드는 주로 정점 셰이더를 정의하고, 모델-뷰-프로젝션(MVP) 행렬을 사용하여 입력 정점의 위치를 변환하는 작업을 수행합니다. 
//셰이더 프로그램은 Direct3D를 사용하여 GPU에서 실행되며, 정점 데이터를 처리하고 화면에 렌더링하는 데 사용됩니다.

//모델-뷰-프로젝션 행렬입니다. 
//이 행렬은 월드 좌표를 클립 좌표로 변환하는 데 사용됩니다. 
//모델 행렬은 객체의 위치, 크기 및 회전을 정의하고, 뷰 행렬은 카메라의 위치와 방향을 정의하며, 프로젝션 행렬은 3D 공간을 2D 화면으로 변환합니다.
matrix MVP;


struct VS_OUT
{
	float4 pos : POSITION;
	float4 col : COLOR;
};

// float4 pos : POSITION0: 입력 정점의 위치입니다. POSITION0 세맨틱을 사용하여 이 값이 입력으로 제공됩니다.
VS_OUT mainVS( float4 pos : POSITION0  ) 
{
	VS_OUT outp;
	
	//outp.pos = mul(pos, MVP);: 입력 정점의 위치를 MVP 행렬을 사용하여 변환합니다. 
	//mul 함수는 행렬과 벡터를 곱하여 변환된 좌표를 계산합니다.
	outp.pos =  mul( pos, MVP );
	//outp.col = float4(1, 1, 1, 1);: 정점의 색상을 흰색으로 설정합니다. 이 값은 이후 단계에서 사용됩니다.
	outp.col = vector(1,1,1,1);

	return outp;
}

technique T0
{
	pass P0
	{ 
		VertexShader = compile vs_1_1 mainVS();
	}
}

//이 셰이더는 주로 정점 변환에 초점을 맞추고 있으며, 색상은 모두 흰색으로 설정되어 있습니다. 
//MVP 행렬을 사용하여 모델, 뷰, 프로젝션 변환을 적용하여 정점을 화면 공간으로 변환합니다.






