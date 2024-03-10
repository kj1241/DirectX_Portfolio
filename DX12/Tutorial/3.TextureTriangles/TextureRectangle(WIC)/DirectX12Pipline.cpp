#include "stdafx.h"
#include "DirectX12Pipline.h"
#include "WinAPI.h"


DirectX12Pipline::DirectX12Pipline() : pD3d12Device(nullptr), pCommandQueue(nullptr), pSwapChain(nullptr), pRtvHeap(nullptr), pRenderTargets{ nullptr }, pCommandAllocator(nullptr), pCommandList(nullptr), pFence(nullptr), hFenceEvent(nullptr), pPipelineState(nullptr), 
pRootSignature(nullptr), pVertexBuffer(nullptr), pIndexBuffer(nullptr),
pSrvHeap(nullptr),
pTexture(nullptr),
viewport(0.0f, 0.0f, 0.0f, 0.0f),
scissorRect(0, 0, 0, 0),
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

	if (pTexture != nullptr)
	{
		pTexture->Release();
		pTexture = nullptr;
	}
	
	if (pIndexBuffer != nullptr)
	{
		pIndexBuffer->Release();
		pIndexBuffer = nullptr;
	}

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

	if (pSrvHeap != nullptr)
	{
		pSrvHeap->Release();
		pSrvHeap = nullptr;
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

	delete imageData;
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
		queueDesc.NodeMask = 0;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

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
		swapChainDesc.BufferCount = 2;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Stereo = false;

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

		//텍스쳐를 사용해야되기때문에 SRV을 만듬

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		result = pD3d12Device->CreateDescriptorHeap(&srvHeapDesc, __uuidof(*pSrvHeap), (void**)&pSrvHeap);
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
		ID3DBlob* pSignature = nullptr;
		ID3DBlob* pError = nullptr;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1; //일단 루트 시그니쳐 버전 1.1로 작성하고

		result = pD3d12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)); //지원하는지 확인하자
		if (result < 0) //지원하지 않으면
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0; // 버전 1.0으로 내리고
			D3D12_DESCRIPTOR_RANGE ranges[1] = {};
			ranges[0].NumDescriptors = 1;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[0].BaseShaderRegister = 0;
			ranges[0].RegisterSpace = 0;
			ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParameters[1] = {};
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
			rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.MipLODBias = 0;
			sampler.MaxAnisotropy = 0;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			sampler.MinLOD = 0.0f;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.NumParameters = _countof(rootParameters);
			rootSignatureDesc.pParameters = rootParameters;
			rootSignatureDesc.NumStaticSamplers = 1;
			rootSignatureDesc.pStaticSamplers = &sampler;

			result = D3D12SerializeRootSignature(&rootSignatureDesc, featureData.HighestVersion, &pSignature, &pError);
			if (result < 0)
				return;

			result = pD3d12Device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), __uuidof(*pRootSignature), (void**)&pRootSignature);
			if (result < 0)
				return;

		}
		else //지원하면
		{


			D3D12_DESCRIPTOR_RANGE1 ranges[1] = {};
			ranges[0].NumDescriptors = 1;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			ranges[0].BaseShaderRegister = 0;
			ranges[0].RegisterSpace = 0;
			ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER1 rootParameters[1] = {};
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
			rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			//rootParameters[0].Descriptor = CBV1rootDescriptor;

			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.MipLODBias = 0;
			sampler.MaxAnisotropy = 0;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			sampler.MinLOD = 0.0f;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.NumParameters = _countof(rootParameters);
			rootSignatureDesc.pParameters = rootParameters;
			rootSignatureDesc.NumStaticSamplers = 1;
			rootSignatureDesc.pStaticSamplers = &sampler;


			D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionRootSignatureDesc = {};
			versionRootSignatureDesc.Desc_1_1 = rootSignatureDesc;
			versionRootSignatureDesc.Version = featureData.HighestVersion;

			result = D3D12SerializeVersionedRootSignature(&versionRootSignatureDesc, &pSignature, &pError);
			if (result < 0)
				return;


			result = pD3d12Device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), __uuidof(*pRootSignature), (void**)&pRootSignature);
			if (result < 0)
				return;
		}

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
			//{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
		renderTargetBlendDesc.BlendEnable = false;
		renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		renderTargetBlendDesc.LogicOpEnable = false;

		//그래픽 상태 파이프라인(PSO)정의 및 설정
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = pRootSignature;
		//psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
		psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
		psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
		//psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
		psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
		//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT)
		psoDesc.RasterizerState.MultisampleEnable = false;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.RasterizerState.FrontCounterClockwise = false;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.AntialiasedLineEnable = false;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		//psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.BlendState.AlphaToCoverageEnable = false;
		psoDesc.BlendState.IndependentBlendEnable = false;
		psoDesc.BlendState.RenderTarget[0] = renderTargetBlendDesc;

		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
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
		////커멘드 리스트로 더 이상 할 것이 없으면 닫아 놓는다. 
		//result = pCommandList->Close();
		//if (result < 0)
		//	return;
	}

	/// <summary>
	/// 버택스 버퍼 만들기
	/// </summary>
	{

		Vertex triangleVertices[] =
		{
			{ { -0.25f, 0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f } },
			{ { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f } },
			{ { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f } },
			{ { 0.25f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f } }
		};

		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(triangleVertices);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

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
		result = pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (result < 0)
			return;
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		pVertexBuffer->Unmap(0, nullptr);

		//정점 버퍼 뷰를 초기화
		vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = sizeof(triangleVertices);
	}


	/// <summary>
	///인덱스 버퍼 만들기
	/// </summary>
	{
		uint32_t indices[] = { 0, 1, 2 ,  0,3,1 };

		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(indices);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		result = pD3d12Device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(*pIndexBuffer), (void**)&pIndexBuffer);
		if (result < 0)
			return;

		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		result = pIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		if (result < 0)
			return;
		memcpy(pVertexDataBegin, indices, sizeof(indices));
		pIndexBuffer->Unmap(0, nullptr);

		indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = sizeof(indices);
	}
	

	/// <summary>
	/// 텍스쳐 만들기
	/// </summary>
	ID3D12Resource* pTextureUploadHeap;

	{
		//TexMetadata metadata = {};
		//ScratchImage scratchImg = {};
		//result = LoadFromWICFile(L"../img/background.png", WIC_FLAGS_NONE, &metadata, scratchImg);
		//auto img = scratchImg.GetImage(0, 0, 0);


		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_CUSTOM;  //D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 0;//1;
		prop.VisibleNodeMask = 0;// 1;


		D3D12_RESOURCE_DESC desc = {};
		//desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //D3D12_RESOURCE_DIMENSION_BUFFER;
		//desc.Alignment = 0;
		//desc.Width = TextureWidth;
		//desc.Height = TextureHeight;
		//desc.DepthOrArraySize = 1;
		//desc.MipLevels = 1;
		//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_UNKNOWN;
		//desc.SampleDesc.Count = 1;
		//desc.SampleDesc.Quality = 0;
		////desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		//desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
		//desc.Alignment = 0;
		//desc.Width = static_cast<UINT>(metadata.width);
		//desc.Height = static_cast<UINT>(metadata.height);
		//desc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
		//desc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
		//desc.Format = metadata.format;//DXGI_FORMAT_UNKNOWN;
		//desc.SampleDesc.Count = 1;
		//desc.SampleDesc.Quality = 0;
		//desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		//desc.Flags = D3D12_RESOURCE_FLAG_NONE;


		int imageBytesPerRow;
		int imageSize = LoadImageDataFromFile(&imageData, desc, L"../img/background.png", imageBytesPerRow);

		result = pD3d12Device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			__uuidof(*pTexture), (void**)&pTexture);
		if (result < 0)
			return;


		/*result = pTexture->WriteToSubresource(0,
			nullptr,
			img->pixels,
			static_cast<UINT>(img->rowPitch),
			static_cast<UINT>(img->slicePitch)
		);*/

		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = GetRequiredIntermediateSize(pTexture, 0, 1);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		result = pD3d12Device->CreateCommittedResource(
			//&keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			//&keep(CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(pTexture, 0, 1))),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(*pTextureUploadHeap), (void**)&pTextureUploadHeap);
		if (result < 0)
			return;

		//중간 업로드 힙에 데이터를 복사한 다음 복사를 예약
		//업로드 힙에서 Texture2D로
		//std::vector<UINT8> texture = GenerateTextureData();


		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &imageData[0];
		textureData.RowPitch = imageBytesPerRow;  //TextureWidth * TexturePixelSize;
		textureData.SlicePitch = imageBytesPerRow * desc.Height; //textureData.RowPitch * TextureHeight;
		UpdateSubresources(pCommandList, pTexture, pTextureUploadHeap, 0, 0, 1, &textureData);

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = pTexture;
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		pCommandList->ResourceBarrier(1, &BarrierDesc);


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		pD3d12Device->CreateShaderResourceView(pTexture, &srvDesc, pSrvHeap->GetCPUDescriptorHandleForHeapStart());

	}
	//	명령 목록을 닫고 실행하여 초기 GPU 설정을 시작합니다.
	result = pCommandList->Close();
	if (result < 0)
		return;

	ID3D12CommandList* ppCommandLists[] ={ pCommandList };
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	pTextureUploadHeap->Release();
	pTextureUploadHeap = nullptr;

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

	ID3D12DescriptorHeap* ppHeaps[] = { pSrvHeap };
	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	pCommandList->SetGraphicsRootDescriptorTable(0, pSrvHeap->GetGPUDescriptorHandleForHeapStart());

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
	pCommandList->SetPipelineState(pPipelineState);
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
	pCommandList->IASetIndexBuffer(&indexBufferView); //인덱스 버퍼
	//pCommandList->DrawInstanced(6, 1, 0, 0);//점 3개 인스턴스1개
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

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

std::vector<UINT8> DirectX12Pipline::GenerateTextureData()
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // 체크보드 텍스처의 셀 너비입니다.
	const UINT cellHeight = TextureWidth >> 3;    // 체커보드 텍스처의 셀 높이입니다.
	const UINT textureSize = rowPitch * TextureHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
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

int DirectX12Pipline::LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
	int hr;
	IWICImagingFactory* wicFactory=nullptr; 	//디코더와 프레임을 생성하려면 인스턴스 필요
	
	//로드하는 이미지마다 다르기 때문에 재설정
	IWICBitmapDecoder* wicDecoder = nullptr;
	IWICBitmapFrameDecode* wicFrame = nullptr;
	IWICFormatConverter* wicConverter = nullptr;

	bool imageConverted = false;

	if (wicFactory == nullptr)
	{
		CoInitialize(nullptr);  //초기화

		// WIC 팩토리 생성
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			__uuidof(*wicFactory), (void**)&wicFactory);
		if (hr < 0)
			return 0;

		hr = wicFactory->CreateFormatConverter(&wicConverter);
		if (hr < 0)
			return 0;
	}

	//이미지 디코더 로드
	hr = wicFactory->CreateDecoderFromFilename(
		filename,							//이미지 파일 이름
		nullptr,                            //공급업체 ID (이거 바이너리 해더파일에 있는데 보통은 안쓰기 때문에 nullptr 설정)
		GENERIC_READ,						// 어떻게 사용할 것인지
		WICDecodeMetadataCacheOnLoad,		// 임의의 시간때 사용하는 것보다 지그 즉시 메타데이터를 캐쉬
		&wicDecoder							// 생성될 WIC 디코더
	);
	if (hr < 0)
		return 0;

	// 디코더 이미지 가저오기 (프레임이 디코딩됨)
	hr = wicDecoder->GetFrame(0, &wicFrame);
	if (hr < 0)
		return 0;

	// 이미지의 WIC 픽셀 형식을 가저옴
	WICPixelFormatGUID pixelFormat;
	hr = wicFrame->GetPixelFormat(&pixelFormat);
	if (hr < 0)
		return 0;

	// 이미지의 크기를 가저옴
	UINT textureWidth, textureHeight;
	hr = wicFrame->GetSize(&textureWidth, &textureHeight);
	if (hr < 0)
		return 0;

	//sRGB 유형에 대해서 처리하지 않음으로 필요하면 직접 다시 만들어야됨

	// WIC 픽셀 형식을 dxgi 픽셀형식으로 변환
	DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	// 이미지 형식이 지원되는 dixg형식이아니면 변환 시도
	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		// 현재 이미지 형식에서 dxgi 호환 WIC 형식을 가져옵
		WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);
		if (convertToPixelFormat == GUID_WICPixelFormatDontCare) // dxgi 호환 형식이 발견되지 않은 경우 반환
			return 0;

		dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);	// dxgi 형식 설정

		// dxgi 호환 형식으로 변환할수 있는지 확인
		BOOL canConvert = FALSE;
		hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
		if (hr < 0 || !canConvert) 
			return 0;

		// 변환을 수행합니다(wicConverter에는 변환된 이미지가 포함)
		hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (hr < 0)
			return 0;

		// wicConverter에서 이미지 데이터를 가져오는 방법을 알기 위해서 플래그 전환
		// 아니면 wicFrame에서 가저와야됨
		imageConverted = true;
	}

	int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // 픽셀당 비트수
	bytesPerRow = (textureWidth * bitsPerPixel) / 8; //이미지 데이터의 각 행에 있는 바이트 수
	int imageSize = bytesPerRow * textureHeight; // 총 이미지 크기

	//원시 이미지 데이터에 충분한 메모리를 할당하고 해당 메모리를 가리키도록 imageData를 설정
	*imageData = new BYTE[imageSize];

	// 	새로 할당된 메모리(imageData)에 원시 이미지 데이터를 복사(디코딩)
	if (imageConverted)
	{
		//이미지 형식을 변환해야 하는 경우 wic 변환기에는 변환된 이미지가 포함
		hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (hr < 0)
			return 0;
	}
	else
	{
		//변환할 필요가 없습니다.wic 프레임에서 데이터를 복사하기만 하면됨
		hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (hr < 0)
			return 0;
	}

	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0; // 0, 4KB, 64KB 또는 4MB일 수 있습니다. 0은 런타임이 64KB와 4MB(멀티 샘플링 텍스처의 경우 4MB) 사이에서 결정
	resourceDescription.Width = textureWidth; 
	resourceDescription.Height = textureHeight; 
	resourceDescription.DepthOrArraySize = 1; //3D 이미지인 경우 3D 이미지의 깊이입니다.그렇지 않으면 1D 또는 2D 텍스처 배열(이미지가 하나뿐이므로 1로 설정)
	resourceDescription.MipLevels = 1; //밉맵 수. 이 텍스처에 대한 밉맵을 생성하지 않으므로 레벨이 하나만 있음
	resourceDescription.Format = dxgiFormat; //이것은 이미지의 dxgi 형식입니다(픽셀 형식).
	resourceDescription.SampleDesc.Count = 1; // 이것은 픽셀당 샘플 수입니다. 우리는 단지 1개의 샘플
	resourceDescription.SampleDesc.Quality = 0; // 샘플의 품질 수준입니다. 높을수록 품질은 좋아지지만 성능떨어짐
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; //픽셀의 배열입니다.알 수 없음으로 설정하면 알아서 가장 효율적인 것을 선택해줌
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // 플레그 없음

	// 이미지의 크기를 반환합니다. 작업이 끝나면 이미지를 삭제하는 것을 잊지 마세요
	wicDecoder->Release();
	wicDecoder = nullptr;

	wicFrame->Release();
	wicFrame = nullptr;

	wicConverter->Release();
	wicConverter = nullptr;

	return imageSize;
}

//wic 형식과 동등한 dxgi 형식
DXGI_FORMAT DirectX12Pipline::GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) 
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) 
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) 
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) 
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) 
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) 
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) 
		return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102)
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551)
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565)
		return DXGI_FORMAT_B5G6R5_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat)
		return DXGI_FORMAT_R32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) 
		return DXGI_FORMAT_R16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGray)
		return DXGI_FORMAT_R16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) 
		return DXGI_FORMAT_R8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) 
		return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}

//다른 WIC 형식에서 dxgi 호환 WIC 형식을 가져옴.
WICPixelFormatGUID DirectX12Pipline::GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) 
		return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) 
		return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) 
		return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) 
		return GUID_WICPixelFormat16bppGrayHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)
		return GUID_WICPixelFormat32bppGrayFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)
		return GUID_WICPixelFormat16bppBGRA5551;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)
		return GUID_WICPixelFormat32bppRGBA1010102;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) 
		return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)
		return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)
		return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)
		return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)
		return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) 
		return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)
		return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) 
		return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) 
		return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) 
		return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}

int DirectX12Pipline::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) 
		return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) 
		return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) 
		return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) 
		return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) 
		return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) 
		return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
		return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) 
		return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) 
		return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)
		return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)
		return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) 
		return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM)
		return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) 
		return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) 
		return 8;
}
