#include "stdafx.h"
#include "DirectX12Pipline.h"
#include "WinAPI.h"


DirectX12Pipline::DirectX12Pipline() : pD3d12Device(nullptr), pCommandQueue(nullptr), pSwapChain(nullptr), pRtvHeap(nullptr), pRenderTargets{nullptr}, pCommandAllocator(nullptr), pCommandList(nullptr), pFence(nullptr), hFenceEvent(nullptr), pPipelineState(nullptr),result(0), frameIndex(0), rtvDescriptorSize(0), fenceValue(0)
{
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

    // 백퍼가 랜더링 대상으로 사용됨 
    pCommandList->ResourceBarrier(1, &keep(CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

    // 커멘드 기록
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

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
