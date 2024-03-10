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
        XMFLOAT2 uv;
    };

    static const UINT FrameCount = 2;
    const UINT TextureWidth = 256;
    const UINT TextureHeight = 256;
    const UINT TexturePixelSize = 4;

    WCHAR assetsPath[512];

    UINT width;
    UINT height;
    float aspectRatio;

    bool useWarpDevice;
    int result;

    ID3D12Device* pD3d12Device;
    ID3D12CommandQueue* pCommandQueue;
    IDXGISwapChain3* pSwapChain;
    ID3D12PipelineState* pPipelineState;
    ID3D12Resource* pRenderTargets[FrameCount];
    ID3D12CommandAllocator* pCommandAllocator;
    ID3D12GraphicsCommandList* pCommandList;
    ID3D12DescriptorHeap* pRtvHeap;
    ID3D12DescriptorHeap* pSrvHeap;
    ID3D12Fence* pFence;
    ID3D12RootSignature* pRootSignature;
    HANDLE hFenceEvent;
    UINT64 fenceValue;
    UINT frameIndex;
    UINT rtvDescriptorSize;

    ID3D12Resource* pVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    ID3D12Resource* pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    ID3D12Resource* pTexture;


    CD3DX12_VIEWPORT viewport;
    CD3DX12_RECT scissorRect;

    BYTE* imageData = {};

    void InitSize();
    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
    std::vector<UINT8> GenerateTextureData();

    void GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);
    void GetAssetFullPath(LPCWSTR assetName,  wchar_t* result);
    void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize);
    
    int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);
    DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
    WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
    int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
};