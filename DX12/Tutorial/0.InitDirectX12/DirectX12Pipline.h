#pragma once

class DirectX12Pipline
{
public:
	DirectX12Pipline();
	~DirectX12Pipline();

	void OnInit();
	void OnUpdate();
	void OnRender();
	void OnDestroy();

private:
    static const UINT FrameCount = 2;

    ID3D12Device* pD3d12Device;
    ID3D12CommandQueue* pCommandQueue;

    IDXGISwapChain3* pSwapChain;


    ID3D12DescriptorHeap* pRtvHeap;
    UINT rtvDescriptorSize;

    ID3D12Resource *pRenderTargets[FrameCount];
    ID3D12CommandAllocator* pCommandAllocator;

    ID3D12GraphicsCommandList* pCommandList;
    ID3D12Fence* pFence;

    ID3D12PipelineState* pPipelineState;

    HANDLE hFenceEvent;
    UINT64 fenceValue;

    UINT frameIndex;



    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();

    bool useWarpDevice;

    void GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);

    int result;

};