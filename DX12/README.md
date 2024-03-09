---

# DirectX 12 튜토리얼 코드(DirectX 12 tutorial)

작성자의 블로그 [Dev_Programming](https://kj1241.github.io/)의 DirectX 12 튜토리얼 샘플 코드입니다.  
This is the DirectX 12 tutorial sample code from the KyeoungJu blog [Dev_Programming](https://kj1241.github.io/).  

- 언어: C++
- 엔진: DirectX 12
- 빌드: Visual Studio 2019 / Window 10
- CPU: intel  i7-4720HQ
- GPU: Nvidia 950M
- 작성 시작일: 2024.03.01 ~

#샘플(Sample)

샘플의 숫자는 순서 이름은 해당되는 파트의 주요 핵심단어 입니다.  
자세한 내용은 블로그에서 설명하겠습니다.  
도큐멘트 작성은 비정기적이며 테스트 코드 작성보다 느릴 수 있습니다.  

The number of samples is the order and the name is the main keyword of the corresponding part.  
Detailed documentation will be provided on the blog.  
Writing documentation is irregular and can be slower than writing test code.  

## [0.InitDirectX12](Tutorial/0.InitDirectX12)

WinAPI를 이용하여 창을 설정합니다. 또한 Direct3D 12를 초기화 및 기본 렌더링 파이프라인에 대해서 설명하고 있습니다.  
Microsoft의 샘플 코드를 참조했습니다.  
Set the window using WinAPI. It also explains Direct3D 12 initialization and the basic rendering pipeline.  
Referenced sample code from Microsoft.  

## [1.DrawTriangles](Tutorial/1.DrawTriangles)

평면의 기초인 삼각형을 렌더링하기 위해 정점을 수동으로 정의합니다.  
Manually define vertices to render triangles that are the basis of a plane.  

**추가) DrawRectangle:**
위의 삼각형을 그리는 작성코드에서 확장하여 사각형을 그리는 코드를 작성하였습니다.  
 have written code to draw a square by extending the code for drawing a triangle above.  

## [2.IndexTriangles](Tutorial/2.IndexTriangles)

최적화를 위해 인덱스를 사용하여 삼각형을 그렸습니다.  
For optimization purposes, triangles were drawn using indices.   

**추가) IndexRectangle:**
위의 삼각형을 그리는 작성코드에서 확장하여 사각형을 그리는 코드를 작성하였습니다.  
have written code to draw a square by extending the code for drawing a triangle above.  

## [3.TextureTriangles](Tutorial/3.TextureTriangles)

정점 삼각형 코드에 uv좌표를 사용하여 체크무늬를 입혔습니다.  
Add a checkered pattern to the vertex triangle code using uv coordinates.  

**추가) TextureRectangle(DirectXTex):**
위의 코드를 확장하여 DirectXTex를 사용하여 png 텍쳐를 입혔습니다.  
Extend the above code to apply a png texture using DirectXTex.  




