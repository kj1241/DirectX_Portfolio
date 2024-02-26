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

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

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

    CD3DX12_VIEWPORT viewport;
    CD3DX12_RECT scissorRect;

    ID3D12RootSignature* pRootSignature;

    ID3D12Resource* pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    float aspectRatio;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();

    bool useWarpDevice;

    void GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);
    std::wstring GetAssetFullPath(LPCWSTR assetName);


    std::wstring assetsPath;
    int result;

};