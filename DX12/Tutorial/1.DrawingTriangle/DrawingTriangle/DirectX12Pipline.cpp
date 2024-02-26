#include "stdafx.h"
#include "DirectX12Pipline.h"
#include "WinAPI.h"

static void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    WCHAR* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

DirectX12Pipline::DirectX12Pipline() : pD3d12Device(nullptr), pCommandQueue(nullptr), pSwapChain(nullptr), pRtvHeap(nullptr), pRenderTargets{ nullptr }, pCommandAllocator(nullptr), pCommandList(nullptr), pFence(nullptr), hFenceEvent(nullptr), pPipelineState(nullptr), result(0), frameIndex(0), rtvDescriptorSize(0), fenceValue(0),
pRootSignature(nullptr), pVertexBuffer(nullptr),
viewport(0.0f, 0.0f, static_cast<float>(1600), static_cast<float>(900)),
scissorRect(0, 0, static_cast<LONG>(1600), static_cast<LONG>(900))
{
    WCHAR tempAssetsPath[512];
    GetAssetsPath(tempAssetsPath, _countof(tempAssetsPath));
    assetsPath = tempAssetsPath;

    aspectRatio = static_cast<float>(1600) / static_cast<float>(900);
}

DirectX12Pipline::~DirectX12Pipline()
{
}

void DirectX12Pipline::OnInit()
{
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
    result =pSwapChain->Present(1, 0);
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

    if (pVertexBuffer != nullptr)
    {
        pVertexBuffer->Release();
        pVertexBuffer = nullptr;
    }

    if (pRootSignature != nullptr)
    {
        pRootSignature->Release();
        pRootSignature = nullptr;
    }

    if (pFence != nullptr)
    {
        pFence->Release();
        pFence = nullptr;
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


// 랜더링 파이프라인 로드
void DirectX12Pipline::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // 디버그 레이어를 활성화합니다(그래픽 도구의 "선택적 기능" 필요).
    // 참고: 장치 생성 후 디버그 계층을 활성화하면 활성 장치가 무효화됩니다.
    {
        ID3D12Debug* pDebugController = nullptr;
        result = D3D12GetDebugInterface(__uuidof(*pDebugController), (void**)&pDebugController);
        if (result)
        {
            pDebugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG; // 추가 디버그 레이어를 활성화합니다.
        }
        pDebugController->Release();
    }
#endif
    IDXGIFactory4* pFactory = nullptr;
     result = CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(*pFactory), (void**)&pFactory);
    if (result < 0)
        return;


    if (useWarpDevice)
    {
        IDXGIAdapter *pWarpAdapter=nullptr;

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

        GetHardwareAdapter(pFactory, &pHardwareAdapter);
        result = D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(*pD3d12Device), (void**)&pD3d12Device);
        if (result < 0)
            return;

        pHardwareAdapter->Release();
        pHardwareAdapter = nullptr;
    }

    // 커멘드 큐를 생성하고 설정
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
 
    result = pD3d12Device->CreateCommandQueue(&queueDesc, __uuidof(*pCommandQueue), (void**)&pCommandQueue);
    if (result < 0)
        return;

    // 스왑체인(백버퍼)을 생성하고 설정
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = WinAPI::pWinAPI->GetWidth();
    swapChainDesc.Height = WinAPI::pWinAPI->GetHeigth();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1 *tempSwapChain;
    result =pFactory->CreateSwapChainForHwnd(
        pCommandQueue,        // 스왑체인은 강재로 플러시를 하기위해서는 대기열이 필요
        WinAPI::pWinAPI->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &tempSwapChain
    );
    if (result < 0)
        return;
    
    
    result = pFactory->MakeWindowAssociation(WinAPI::pWinAPI->GetHwnd(), DXGI_MWA_NO_ALT_ENTER);
    if (result < 0)
        return;
    //tempSwapChain.As(&pSwapChain);
    result = tempSwapChain->QueryInterface(__uuidof(pSwapChain), (void**)&pSwapChain);
    if (result < 0)
        return;


    frameIndex = pSwapChain->GetCurrentBackBufferIndex();

    // 힙을 정의
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
            result =pSwapChain->GetBuffer(i, __uuidof(*pRenderTargets[i]), (void**)&pRenderTargets[i]);
            pD3d12Device->CreateRenderTargetView(pRenderTargets[i], nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);
        }
    }
    result=pD3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(*pCommandAllocator), (void**)&pCommandAllocator);
    if (result < 0)
        return;

    tempSwapChain->Release();
    tempSwapChain = nullptr;
    pFactory->Release();
    pFactory = nullptr;
}

//에셋을 로드
void DirectX12Pipline::LoadAssets()
{
    ///////////
    // 빈 루트시그널 만들기
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        ID3DBlob* pSignature=nullptr;
        ID3DBlob* pError=nullptr;

        result= D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError);
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
    // 파이프라인 스테이트 만들고 컴파일링과 로딩쉐이더를 포함
    {
        ID3DBlob* pVertexShader = nullptr;
        ID3DBlob* pPixelShader = nullptr;

#if defined(_DEBUG)
        //쉐이더 디버깅 그래픽 디버깅툴에 포함
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"../../shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"../../shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr));
        // 버텍스 레이아웃 포함
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        //그래픽 파이프라인 상태 개체(PSO)를 설명하고 생성
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = pRootSignature;
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader);
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader);
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        result= pD3d12Device->CreateGraphicsPipelineState(&psoDesc, __uuidof(*pPipelineState), (void**)&pPipelineState);
        if (result < 0)
            return;

        if (pVertexShader != nullptr)
        {
            pVertexShader->Release();
            pVertexShader = nullptr;
        }
        if (pPixelShader != nullptr)
        {
            pPixelShader->Release();
            pPixelShader = nullptr;
        }
    }
    ////////////

    //커멘드 리스트를 만든다.
    result = pD3d12Device->CreateCommandList(0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        pCommandAllocator,
        nullptr,
        __uuidof(*pCommandList), (void**)&pCommandList);
    if (result < 0)
        return;
    // 명령어 목록이 생성되는데 아무것도 없음
    // 메인루프는 닫힐것으로 예상함으로 지금 닫음
    result= pCommandList->Close();
    if (result < 0)
        return;
    /////////////////

    {
        // 지오메트릭스 삼각형 만들기
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };
        const UINT vertexBufferSize = sizeof(triangleVertices);
        // 참고: 업로드 힙을 사용하여 수직 버퍼와 같은 정적 데이터를 전송하는 것은 아님
            // 추천합니다. GPU에 필요할 때마다 업로드 힙이 마샬링됨
            // 위에. 기본 힙 사용량을 읽으셈. 여기서는 업로드 힙이 사용.
            // 코드가 단순하고 실제로 전송할 vert가 거의 없기 때문
        result = pD3d12Device->CreateCommittedResource(
            &keep(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            &keep(CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&pVertexBuffer));
        if (result < 0)
            return;

        // 삼각형 데이터를 정점 버퍼에 복사
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        //우리는 CPU의 이 리소스를 읽을 의도가 없음
        result = pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (result < 0)
            return;
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        pVertexBuffer->Unmap(0, nullptr);
        // 정점 버퍼 보기를 초기화
        vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = vertexBufferSize;
    }
    //////////


    // 동기화 개체 만들기
    {
        result=pD3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(*pFence), (void**)&pFence);
        if (result < 0)
            return;

        fenceValue = 1;

        // 프레임 동기화에 사용할 이벤트 핸들을 만들기
        hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (hFenceEvent == nullptr)
        {
            result=HRESULT_FROM_WIN32(GetLastError());
            if (result < 0)
                return;
        }
    }
}

void DirectX12Pipline::PopulateCommandList()
{
    // 커멘드 리스트는 할당자는 연결된 경우에만 재설정
    // 커멘드 리스트는 GPU에서 실행종료 하지만 앱은 실행
    // GPU 실행 진행 상황을 결정하는 펜스
    result= pCommandAllocator->Reset();
    if (result < 0)
        return;

    // 특정 커멘드 리스트에서 ExecuteCommandList()가 호출되는 경우
    // 해당 명령 목록은 언제든지 재설정될 수 있으며 이전에 재설정되어야 함.
    // 다시 플레이중 
    result =pCommandList->Reset(pCommandAllocator, pPipelineState);
    if (result < 0)
        return;
  
    ////////////
    // Set necessary state.
    pCommandList->SetGraphicsRootSignature(pRootSignature);
    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    /////////////

    // 백퍼가 랜더링 대상으로 사용됨 
    pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    
    /// <summary>
    /// ////
    /// </summary>
    pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    ///////

    // 커멘드 기록
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    /// <summary>
    /// ///////
    /// </summary>
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    pCommandList->DrawInstanced(3, 1, 0, 0);
    ///////////////


    // 이제 백버퍼가 화면으로 사용됨
    pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
    ThrowIfFailed(pCommandList->Close());
}

void DirectX12Pipline::WaitForPreviousFrame()
{
    // 계속하기 전에 프레임이 완료될 때까지 기다리는 것은 최선의 방법이 아닙
    // 단순화를 위해 이렇게 구현한 코드
    // 샘플은 효율적인 리소스 사용을 위해 펜스를 사용하는 방법을 보여줌
    // GPU 활용도를 최대화합니다.

    // 신호를 보내고 펜스값 증가
    const UINT64 fence = fenceValue;
    ThrowIfFailed(pCommandQueue->Signal(pFence, fence));
    fenceValue++;

    // 이전 프레임이 완료될때까지 기다림
    if (pFence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(pFence->SetEventOnCompletion(fence, hFenceEvent));
        WaitForSingleObject(hFenceEvent, INFINITE);
    }

    frameIndex = pSwapChain->GetCurrentBackBufferIndex();
}

void DirectX12Pipline::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr; //초기값

    IDXGIAdapter1* pAapter=nullptr;
    IDXGIFactory6* pFactory6=nullptr;
    int result = 0;

    result = pFactory->QueryInterface(__uuidof(*pFactory6), (void**)&pFactory6);

    if (result>=0)
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

            // 장치가 12를지원하는 확인하지만 생성하지 않는다
            if (SUCCEEDED(D3D12CreateDevice(pAapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
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

std::wstring DirectX12Pipline::GetAssetFullPath(LPCWSTR assetName)
{
    return assetsPath + assetName;
}

