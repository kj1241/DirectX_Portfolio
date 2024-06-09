#include "stdafx.h"
#include "DirectX12Pipline.h"
#include "WinAPI.h"


DirectX12Pipline::DirectX12Pipline() : pD3d12Device(nullptr), pCommandQueue(nullptr), pSwapChain(nullptr), pRtvHeap(nullptr), pRenderTargets{ nullptr }, pCommandAllocator(nullptr), pCommandList(nullptr), pFence(nullptr), hFenceEvent(nullptr), pPipelineState(nullptr), 
pRootSignature(nullptr), pVertexBuffer(nullptr), //pIndexBuffer(nullptr),
//viewport(0.0f, 0.0f, 0.0f, 0.0f),
//scissorRect(0, 0, 0, 0),
result(0), frameIndex(0), rtvDescriptorSize(0), fenceValue(0)
{
	GetAssetsPath(assetsPath, _countof(assetsPath));
}

DirectX12Pipline::~DirectX12Pipline()
{
}

void DirectX12Pipline::OnInit()
{
	InitSize();
	LoadPipeline();
	LoadAssets();

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect = { 0, 0, (int)width, (int)height };

}

void DirectX12Pipline::OnUpdate()
{
}

// 장면을 랜더링
void DirectX12Pipline::OnRender()
{
	// 장명은 렌더링하는데 필요한 모든 커멘드를 커멘드리스트에 기록함
	PopulateCommandList();

	// 커멘드리스트를 실행
	ID3D12CommandList* ppCommandLists[] = { pCommandList };
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// 프레임을 제시
	result = pSwapChain->Present(1, 0);
	if (result < 0)
		return;

	WaitForPreviousFrame();
}

void DirectX12Pipline::OnDestroy()
{
	// GPU가 더 이상 참조할 리소스를 참조하지 않는지 확인.
   // 소멸자에 의해 정리.
	WaitForPreviousFrame();
	
	CloseHandle(hFenceEvent);

	if (pFence != nullptr)
	{
		pFence->Release();
		pFence = nullptr;
	}

	//if (pIndexBuffer != nullptr)
	//{
	//	pIndexBuffer->Release();
	//	pIndexBuffer = nullptr;
	//}

	if (pVertexBuffer != nullptr)
	{
		pVertexBuffer->Release();
		pVertexBuffer= nullptr;
	}
	
	if (pCommandList != nullptr)
	{
		pCommandList->Release();
		pCommandList = nullptr;
	}

	for (int i = 0; i < FrameCount; ++i)
		if (pRenderTargets[i] != nullptr)
		{
			pRenderTargets[i]->Release();
			pRenderTargets[i] = nullptr;
		}

	if (pCommandAllocator != nullptr)
	{
		pCommandAllocator->Release();
		pCommandAllocator = nullptr;
	}

	if (pRootSignature != nullptr)
	{
		pRootSignature->Release();
		pRootSignature = nullptr;
	}

	if (pRtvHeap != nullptr)
	{
		pRtvHeap->Release();
		pRtvHeap = nullptr;
	}

	if (pSwapChain != nullptr)
	{
		pSwapChain->Release();
		pSwapChain = nullptr;
	}

	if (pCommandQueue != nullptr)
	{
		pCommandQueue->Release();
		pCommandQueue = nullptr;
	}

	if (pD3d12Device != nullptr)
	{
		pD3d12Device->Release();
		pD3d12Device = nullptr;
	}
}

//생성자에서 초기값을 할당해줄수 없는부분들.(순서에 의해서)
void DirectX12Pipline::InitSize()
{
	width = WinAPI::pWinAPI->GetWidth();
	height = WinAPI::pWinAPI->GetHeigth();
	viewport.Width = width;
	viewport.Height = height;
	scissorRect.right = width;
	scissorRect.bottom = height;
	aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

// 랜더링 파이프라인 로드
void DirectX12Pipline::LoadPipeline()
{
	/// <summary>
	/// 디버그 레이어를 활성화합니다(그래픽 도구의 "선택적 기능" 필요).
	/// 참고: 장치 생성 후 디버그 계층을 활성화하면 활성 장치가 무효화됩니다.
	/// </summary>
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ID3D12Debug* pDebugController = nullptr;
		result = D3D12GetDebugInterface(__uuidof(*pDebugController), (void**)&pDebugController);
		if (result)
		{
			pDebugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG; // 추가 디버그 레이어를 활성화합니다.
		}
		pDebugController->Release();
		pDebugController = nullptr;
	}
#endif

	/// <summary>
	/// 하드웨어 그래픽카드 어뎁터 찾기
	/// </summary>
	IDXGIFactory4* pFactory = nullptr;
	result = CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(*pFactory), (void**)&pFactory);
	if (result < 0)
		return;

	if (useWarpDevice)
	{
		IDXGIAdapter* pWarpAdapter = nullptr;
		//연결 가능한 어뎁터 찾기
		pFactory->EnumWarpAdapter(__uuidof(*pWarpAdapter), (void**)&pWarpAdapter);
		result = D3D12CreateDevice(pWarpAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(*pD3d12Device), (void**)&pD3d12Device);
		if (result < 0)
			return;

		pWarpAdapter->Release();
		pWarpAdapter = nullptr;

	}
	else
	{
		IDXGIAdapter1* pHardwareAdapter = nullptr;
		//하드웨어 어뎁터 찾기
		GetHardwareAdapter(pFactory, &pHardwareAdapter);
		result = D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(*pD3d12Device), (void**)&pD3d12Device);
		if (result < 0)
			return;

		pHardwareAdapter->Release();
		pHardwareAdapter = nullptr;
	}

	/// <summary>
	/// 커멘드 큐를 생성하고 설정
	/// </summary>
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		result = pD3d12Device->CreateCommandQueue(&queueDesc, __uuidof(*pCommandQueue), (void**)&pCommandQueue);
		if (result < 0)
			return;
	}

	/// <summary>
	/// 스왑체인(백버퍼)을 생성하고 설정
	/// </summary>
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = WinAPI::pWinAPI->GetWidth();
		swapChainDesc.Height = WinAPI::pWinAPI->GetHeigth();
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		IDXGISwapChain1* tempSwapChain;
		result = pFactory->CreateSwapChainForHwnd(
			pCommandQueue,        //프레임은 반드시 완성되면 선입선출해야됨으로 큐에 쓔셔밖아서 대기열을 만든다.
			WinAPI::pWinAPI->GetHwnd(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&tempSwapChain
		);
		if (result < 0)
			return;

		//전체화면 금지 이게 여기에 쓰이는 이유는 스왑체인 그릴때 화면을 어떻게 그려줄것인지 설정해야 되기 때문에.
		result = pFactory->MakeWindowAssociation(WinAPI::pWinAPI->GetHwnd(), DXGI_MWA_NO_ALT_ENTER);
		if (result < 0)
			return;

		//이런 식으로 작성한 이유? 스왑체인 버전 바꾸기 위해서
		//tempSwapChain.As(&pSwapChain);
		result = tempSwapChain->QueryInterface(__uuidof(pSwapChain), (void**)&pSwapChain);
		if (result < 0)
			return;

		//백버퍼 번호가저오기
		frameIndex = pSwapChain->GetCurrentBackBufferIndex();

		tempSwapChain->Release();
		tempSwapChain = nullptr;
	}

	/// <summary>
	/// 힙을 정의
	/// </summary> 
	{
		//RTV(랜더링 대상 뷰) 설명자 힙을 정의하고 생성
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		result = pD3d12Device->CreateDescriptorHeap(&rtvHeapDesc, __uuidof(*pRtvHeap), (void**)&pRtvHeap); //실제로 쓰는곳
		if (result < 0)
			return;

		rtvDescriptorSize = pD3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// 프레임 리소스 생성
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart());

		// RTV같은 프레임 생성
		for (UINT i = 0; i < FrameCount; ++i)
		{
			result = pSwapChain->GetBuffer(i, __uuidof(*pRenderTargets[i]), (void**)&pRenderTargets[i]);
			pD3d12Device->CreateRenderTargetView(pRenderTargets[i], nullptr, rtvHandle);
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
	}

	/// <summary>
	/// 커멘드 리스트를 할당한다.
	/// </summary>
	{
		result = pD3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(*pCommandAllocator), (void**)&pCommandAllocator);
		if (result < 0)
			return;

	}
	pFactory->Release();
	pFactory = nullptr;
}

//에셋을 로드
void DirectX12Pipline::LoadAssets()
{

	/// <summary>
	/// 루트시그널을 만듬
	/// </summary>
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ID3DBlob* pSignature = nullptr;
		ID3DBlob* pError = nullptr;

		result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError);
		if (result < 0)
			return;

		result = pD3d12Device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), __uuidof(*pRootSignature), (void**)&pRootSignature);
		if (result < 0)
			return;

		if (pSignature != nullptr)
		{
			pSignature->Release();
			pSignature = nullptr;
		}
		if (pError != nullptr)
		{
			pError->Release();
			pError = nullptr;
		}

	}

	/// <summary>
	/// 상태 파이프라인을 만들고 셰이더 컴파일 및 로드
	/// </summary>
	{
		ID3DBlob* vertexShader = nullptr;
		ID3DBlob* pixelShader = nullptr;


	//디버깅 모드를 사용하여 셰이더 디버깅
	#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#else
		UINT compileFlags = 0;
	#endif

		wchar_t assetPath[512] = {};
		GetAssetFullPath(L"../../BasicVertexShader.hlsl", assetPath);
		result = D3DCompileFromFile(assetPath, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
		if (result < 0)
			return;

		GetAssetFullPath(L"../../BasicPixelShader.hlsl", assetPath);
		result = D3DCompileFromFile(assetPath, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
		if (result < 0)
			return;

		// 정점 레이아웃을 정의
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		//그래픽 상태 파이프라인(PSO)정의 및 설정
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = pRootSignature;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		result = pD3d12Device->CreateGraphicsPipelineState(&psoDesc, __uuidof(*pPipelineState), (void**)&pPipelineState);
		if (result < 0)
			return;

		if (vertexShader != nullptr)
		{
			vertexShader->Release();
			vertexShader = nullptr;
		}
		if (pixelShader != nullptr)
		{
			pixelShader->Release();
			pixelShader = nullptr;
		}

	}

	/// <summary>
	///커멘드 리스트를 만듬
	/// </summary>
	{
		result = pD3d12Device->CreateCommandList(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			pCommandAllocator,
			pPipelineState,
			__uuidof(*pCommandList), (void**)&pCommandList);
		if (result < 0)
			return;
		//커멘드 리스트로 더 이상 할 것이 없으면 닫아 놓는다. 
		result = pCommandList->Close();
		if (result < 0)
			return;
	}


	Model.LoadModel(L"cube.txt");



	/// <summary>
	/// 버택스 버퍼 만들기
	/// </summary>
	{
		//Vertex triangleVertices[] =
		//{
		//	{ { -0.25f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		//	{ { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		//	//{ { 0.25f, 0.25f * aspectRatio, 0.0f }, { 0.0f, 0.5f, 0.5f, 1.0f } }
		//};

		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = Model.GetModelSize();
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		//const UINT vertexBufferSize = sizeof(triangleVertices);
		result = pD3d12Device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(*pVertexBuffer), (void**)&pVertexBuffer);
		if (result < 0)
			return;

		//삼각형 정점데이터를 버퍼에 복사
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // CPU리소스를 읽지 않을꺼기 때문에.
		result =pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (result < 0)
			return;
		memcpy(pVertexDataBegin, Model.GetModel(), Model.GetModelSize());
		pVertexBuffer->Unmap(0, nullptr);

		//정점 버퍼 뷰를 초기화
		vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = Model.GetModelSize();
	}

	////인덱스 버퍼 만들기
	//{
	//	uint32_t indices[] = { 0, 1, 2 ,  0,3,1 };

	//	D3D12_HEAP_PROPERTIES prop = {};
	//	prop.Type = D3D12_HEAP_TYPE_UPLOAD;
	//	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//	prop.CreationNodeMask = 1;
	//	prop.VisibleNodeMask = 1;

	//	D3D12_RESOURCE_DESC desc = {};
	//	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//	desc.Alignment = 0;
	//	desc.Width = sizeof(indices);
	//	desc.Height = 1;
	//	desc.DepthOrArraySize = 1;
	//	desc.MipLevels = 1;
	//	desc.Format = DXGI_FORMAT_UNKNOWN;
	//	desc.SampleDesc.Count = 1;
	//	desc.SampleDesc.Quality = 0;
	//	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	//	result = pD3d12Device->CreateCommittedResource(
	//		&prop,
	//		D3D12_HEAP_FLAG_NONE,
	//		&desc,
	//		D3D12_RESOURCE_STATE_GENERIC_READ,
	//		nullptr, 
	//		__uuidof(*pIndexBuffer), (void**)&pIndexBuffer);
	//	if (result < 0)
	//		return;

	//	UINT8* pVertexDataBegin;
	//	CD3DX12_RANGE readRange(0, 0);
	//	result = pIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	//	if (result < 0)
	//		return;
	//	memcpy(pVertexDataBegin, indices, sizeof(indices));
	//	pIndexBuffer->Unmap(0, nullptr);

	//	indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	//	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	//	indexBufferView.SizeInBytes = sizeof(indices);
	//}				  


	/// <summary>
	/// 울타리 개체 만들기
	/// </summary>
	{
		result = pD3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(*pFence), (void**)&pFence);
		if (result < 0)
			return;

		fenceValue = 1;

		// 프레임 동기화에 사용할 이벤트 핸들을 만들기
		hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hFenceEvent == nullptr)
		{
			result = HRESULT_FROM_WIN32(GetLastError());
			if (result < 0)
				return;
		}

		// 커멘드 리스트가 완료될떄까지 기다림
		WaitForPreviousFrame();
	}
}

void DirectX12Pipline::PopulateCommandList()
{
	// 커멘드 리스트는 할당자는 연결된 경우에만 재설정
	// 커멘드 리스트는 GPU에서 실행종료 하지만 앱은 실행
	// GPU 실행 진행 상황을 결정하는 펜스
	result = pCommandAllocator->Reset();
	if (result < 0)
		return;

	// 특정 커멘드 리스트에서 ExecuteCommandList()가 호출되는 경우
	// 해당 명령 목록은 언제든지 재설정될 수 있으며 이전에 재설정되어야 함.
	// 다시 플레이중 
	result = pCommandList->Reset(pCommandAllocator, pPipelineState);
	if (result < 0)
		return;


	// 상태를 설정
	pCommandList->SetGraphicsRootSignature(pRootSignature); //루트 시그니쳐 설정
	pCommandList->RSSetViewports(1, &viewport);			//뷰포트 
	pCommandList->RSSetScissorRects(1, &scissorRect);     //클러핑 사각형


	//현재 백버퍼 인덱스 가져오기
	frameIndex = pSwapChain->GetCurrentBackBufferIndex();

	// 백퍼가 랜더링 대상으로 사용됨 
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = pRenderTargets[frameIndex];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	pCommandList->ResourceBarrier(1, &BarrierDesc);
	//pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));

	//렌더 타겟 지정
	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize); 
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<ULONG_PTR>(frameIndex * pD3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	pCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

	// 커멘드 기록
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //트라이엥글 리스트로만들기
	pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView); //버텍스버퍼뷰어 크기로
	//pCommandList->IASetIndexBuffer(&indexBufferView); //인덱스 버퍼
	pCommandList->DrawInstanced(Model.GetModelFaceCount(), 1, 0, 0);
	//pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	// 이제 백버퍼가 화면으로 사용됨
	//pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	pCommandList->ResourceBarrier(1, &BarrierDesc);

	result = pCommandList->Close();
	if (result < 0)
		return;
}

void DirectX12Pipline::WaitForPreviousFrame()
{
	// 계속하기 전에 프레임이 완료될 때까지 기다리는 것은 최선의 방법이 아닙
	// 단순화를 위해 이렇게 구현한 코드
	// 샘플은 효율적인 리소스 사용을 위해 펜스를 사용하는 방법을 보여줌
	// GPU 활용도를 최대화합니다.

	// 신호를 보내고 펜스값 증가
	const UINT64 fence = fenceValue;
	result = pCommandQueue->Signal(pFence, fence);
	if (result < 0)
		return;

	fenceValue++;

	// 이전 프레임이 완료될때까지 기다림
	if (pFence->GetCompletedValue() < fence)
	{
		result = pFence->SetEventOnCompletion(fence, hFenceEvent);
		if (result < 0)
			return;
		WaitForSingleObject(hFenceEvent, INFINITE);
	}

	frameIndex = pSwapChain->GetCurrentBackBufferIndex();
}


//하드웨어 어뎁터(그래픽카드)찾아내기
void DirectX12Pipline::GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr; //초기값
	IDXGIAdapter1* pAapter = nullptr;
	IDXGIFactory6* pFactory6 = nullptr;
	int result = 0;

	result = pFactory->QueryInterface(__uuidof(*pFactory6), (void**)&pFactory6);

	if (result >= 0)
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(pFactory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				__uuidof(*pAapter), (void**)&pAapter));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc; // 혹시 스왑체인이 필요할 수 있을지도 모르니
			pAapter->GetDesc1(&desc);



			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// 기본 렌더링 드라이버를 선택하면 안됨
				// 소프트웨어 어뎁터가 필요하면 /warp를 전달함
				continue;
			}

			//자꾸 내장 그래픽카드로 잡아서 짜증나서 엔디비아로 잡게 코드작성
			std::wstring strDesc = desc.Description;
			if (strDesc.find(L"NVIDIA") != std::string::npos) {
				//*ppAdapter = pAapter;
				break;
			}
			//// 하드웨어 지원하는 장비가 존재한다면 리턴한다
			//if (SUCCEEDED(D3D12CreateDevice(pAapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			//{
			//    break;
			//}
		}
	}

	if (pAapter == nullptr) // 혹시 어뎁터가 존재하지 않는다면
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &pAapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{

				continue;
			}

			// 장치가 12를지원하는 확인하지만 생성하지 않는다
			if (SUCCEEDED(D3D12CreateDevice(pAapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = pAapter;

	//if (pAapter != nullptr) {
	//    pAapter->Release();
	//    pAapter = nullptr;
	//}
	if (pFactory6 != nullptr) {
		pFactory6->Release();
		pFactory6 = nullptr;
	}
}

void DirectX12Pipline::GetAssetFullPath(LPCWSTR assetName, wchar_t* result)
{
	result[0] = L'\0';
	wcscpy_s(result, 512, assetsPath);
	wcscat_s(result, 512, assetName);
}


void DirectX12Pipline::GetAssetsPath(WCHAR* path, UINT pathSize)
{
	//GetCurrentDirectory 이거 쓸껄 그랬나?
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}
