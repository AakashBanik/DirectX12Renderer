#pragma once

#include "stdafx.h"

namespace dmp
{
   template <typename StateT>
   class BaseRenderer
   {
   public:
      BaseRenderer() {}

      BaseRenderer(HWND windowHandle, int width, int height) :
         mWindowHandle(windowHandle), mWidth(width), mHeight(height)
      {}

      virtual ~BaseRenderer()
      {
         if (mDevice) flushCommandQueue();
      }

      float getAspectRatio() const
      {
         expectTrue("rhs not 0",
                    mHeight != 0);
         return static_cast<float>(mWidth) / static_cast<float>(mHeight);
      }

      bool isMsaaEnabled() const { return mMsaaEnabled; }
      void setMsaaState(bool state) { mMsaaEnabled = state; }

      HRESULT resize(int width, int height, bool force = false)
      {
         expectTrue("Renderer is ready", mReady);
         expectTrue("mDevice not null", mDevice);
         expectTrue("mSwapChain not null", mSwapChain);
         expectTrue("mWindowHandle not null", mWindowHandle);
         expectTrue("mDirectCmdListAlloc not null", mDirectCmdListAlloc);

         expectRes("resize impl", resizeImpl(width, height, force));

         return S_OK;
      }

      void draw()
      {
         if (!mReady) return;
         expectRes("Draw implementation", drawImpl());
      }

      void update(const StateT & t)
      {
         expectTrue("Update implementation", updateImpl(t));
      }

   protected:

      // update hook
      virtual bool updateImpl(const StateT & t) { return true; }

      // resize hook
      virtual HRESULT resizeImpl(int width, int height, bool force = false)
      {
         // make sure the size actually changed
         if (width == mWidth && height == mHeight && !force) return S_OK;

         mWidth = width;
         mHeight = height;

         flushCommandQueue();
         expectRes("reset command list",
                   mCommandList
                   ->Reset(mDirectCmdListAlloc.Get(),
                           nullptr)); // TODO: need to update when I make PSO?

         for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
         {
            mSwapChainBuffers[i].Reset();
         }
         mDepthStencilBuffer.Reset();

         expectRes("Resize mSwapChain",
                   mSwapChain
                   ->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT,
                                   mWidth,
                                   mHeight,
                                   mBackBufferFormat,
                                   DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

         mCurrentBackBuffer = 0;

         CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap
                                                     ->GetCPUDescriptorHandleForHeapStart());
         for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
         {
            expectRes("Get swapChainBuffer[i]",
                      mSwapChain
                      ->GetBuffer(i,
                                  IID_PPV_ARGS(mSwapChainBuffers[i]
                                               .ReleaseAndGetAddressOf())));

            mDevice->CreateRenderTargetView(mSwapChainBuffers[i].Get(),
                                            nullptr,
                                            rtvHeapHandle);

            rtvHeapHandle.Offset(1, mRtvDescriptorSize);
         }

         D3D12_RESOURCE_DESC dsd;
         dsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
         dsd.Alignment = 0;
         dsd.Width = mWidth;
         dsd.Height = mHeight;
         dsd.DepthOrArraySize = 1;
         dsd.MipLevels = 1;
         dsd.Format = mDepthStencilFormat;
         dsd.SampleDesc.Count = mMsaaEnabled ? 4 : 1;
         dsd.SampleDesc.Quality = mMsaaEnabled ? (mMsaaQualityLevel - 1) : 0;
         dsd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
         dsd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

         D3D12_CLEAR_VALUE clear;
         clear.Format = mDepthStencilFormat;
         clear.DepthStencil.Depth = 1.0f;
         clear.DepthStencil.Stencil = 0;

         expectRes("create depth stencil buffer",
                   mDevice
                   ->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                             D3D12_HEAP_FLAG_NONE,
                                             &dsd,
                                             D3D12_RESOURCE_STATE_COMMON,
                                             &clear,
                                             IID_PPV_ARGS(mDepthStencilBuffer
                                                          .ReleaseAndGetAddressOf())));

         mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(),
                                         nullptr,
                                         depthStencilView());

         mCommandList->ResourceBarrier(1,
                                       &CD3DX12_RESOURCE_BARRIER
                                       ::Transition(mDepthStencilBuffer.Get(),
                                                    D3D12_RESOURCE_STATE_COMMON,
                                                    D3D12_RESOURCE_STATE_DEPTH_WRITE));

         expectRes("close the command list",
                   mCommandList->Close());

         std::array<ID3D12CommandList *, 1>  commandLists = {mCommandList.Get()};
         mCommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()),
                                            commandLists.data());

         flushCommandQueue();

         mViewport.TopLeftX = 0;
         mViewport.TopLeftY = 0;
         mViewport.Width = static_cast<float>(mWidth);
         mViewport.Height = static_cast<float>(mHeight);
         mViewport.MinDepth = 0.0f;
         mViewport.MaxDepth = 1.0f;

         mScissorRect = {0, 0, mWidth, mHeight};

         mP = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(0.25 * DirectX::XM_PI, getAspectRatio(), 1.0f, 1000.0f);

         return S_OK;
      }

      // draw hook
      virtual HRESULT drawImpl() = 0;

      // derived classes should call this in their constructor
      void init()
      {
         expectTrue("mWindowHandle not null", mWindowHandle);
         expectTrue("Init Renderer Base", initBase());
         expectTrue("Init MSAA", initMsaa());
         expectTrue("Init command objects", initCommand());
         expectRes("Init swapchain", recreateSwapChain());
         expectTrue("Create Descriptor Heaps", createDescriptorHeaps());

         expectTrue("init impl", initImpl());

         mReady = true;
         resize(mWidth, mHeight, true);
      }

      HRESULT recreateSwapChain()
      {
         expectTrue("mWindowHandle not null", mWindowHandle);
         expectTrue("mCommandQueue not null", mCommandQueue);

         DXGI_SWAP_CHAIN_DESC sd;

         sd.BufferDesc.Width = mWidth;
         sd.BufferDesc.Height = mHeight;
         sd.BufferDesc.RefreshRate.Numerator = 60;
         sd.BufferDesc.RefreshRate.Denominator = 1;
         sd.BufferDesc.Format = mBackBufferFormat;
         sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
         sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
         sd.SampleDesc.Count = mMsaaEnabled ? 4 : 1;
         sd.SampleDesc.Quality = mMsaaEnabled ? (mMsaaQualityLevel - 1) : 0;
         sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
         sd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
         sd.OutputWindow = mWindowHandle;
         sd.Windowed = true;
         sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
         sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

         expectRes("Recreate swapchain",
                   mDXGIFactory
                   ->CreateSwapChain(mCommandQueue.Get(),
                                     &sd,
                                     mSwapChain.ReleaseAndGetAddressOf()));

         return S_OK;
      }

      bool createDescriptorHeaps() // TODO: candidate for virtual
      {
         D3D12_DESCRIPTOR_HEAP_DESC rhd;
         rhd.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
         rhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
         rhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
         rhd.NodeMask = 0; // single-adaptor
         expectRes("create rtv descriptor heap",
                   mDevice
                   ->CreateDescriptorHeap(&rhd,
                                          IID_PPV_ARGS(mRtvHeap
                                                       .ReleaseAndGetAddressOf())));

         D3D12_DESCRIPTOR_HEAP_DESC dhd;
         dhd.NumDescriptors = 1;
         dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
         dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
         dhd.NodeMask = 0; // single-adaptor
         expectRes("create dsv descriptor heap",
                   mDevice
                   ->CreateDescriptorHeap(&dhd,
                                          IID_PPV_ARGS(mDsvHeap
                                                       .ReleaseAndGetAddressOf())));

         return true;
      }

      // fields

      void flushCommandQueue(std::function<void()> callback = doNothing)
      {
         expectTrue("Fence exists", mFence);
         expectTrue("Command queue exists", mCommandQueue);
         expectTrue("fence val < uint64Max", mFenceVal < std::numeric_limits<uint64_t>::max());

         ++mFenceVal;

         expectRes("Signal fence",
                   mCommandQueue->Signal(mFence.Get(), mFenceVal));

         callback(); // potentially do something useful rather than waiting

         if (mFence->GetCompletedValue() < mFenceVal)
         {
            auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            expectRes("Set event to wait on while queue flushes",
                      mFence->SetEventOnCompletion(mFenceVal, event));

            WaitForSingleObject(event, INFINITE);
            expectRes("Close event handle",
                      CloseHandle(event));
         }
      }

      ID3D12Resource * currentBackBuffer()
      {
         return mSwapChainBuffers[mCurrentBackBuffer].Get();
      }

      D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView() const
      {
         return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap
                                              ->GetCPUDescriptorHandleForHeapStart(),
                                              mCurrentBackBuffer,
                                              mRtvDescriptorSize);
      }

      D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView() const
      {
         return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
      }

      HWND mWindowHandle;
      int mWidth;
      int mHeight;

      Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
      Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
      uint64_t mFenceVal = 0;
      Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

      UINT mRtvDescriptorSize = 0;
      UINT mDsvDescriptorSize = 0;
      UINT mCbvSrvUavDescriptorSize = 0;

      DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      UINT mMsaaQualityLevel = 0;
      bool mMsaaEnabled = false;

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

      Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
      UINT mCurrentBackBuffer = 0;
      std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SWAP_CHAIN_BUFFER_COUNT> mSwapChainBuffers;

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

      DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
      D3D12_VIEWPORT mViewport;
      D3D12_RECT mScissorRect;

      DirectX::SimpleMath::Matrix mP; // TODO: Do I care about orthographic projection?

   private:
      // this will be called by init and need not be called by the derived class
      virtual bool initImpl() = 0;

      bool mReady = false;
      bool initBase()
      {
         using namespace Microsoft::WRL;
#if defined(DEBUG) || defined(_DEBUG) 
         // Enable the D3D12 debug layer.
         {
            ComPtr<ID3D12Debug> debugController;
            expectRes("create debug controller",
                      D3D12GetDebugInterface(IID_PPV_ARGS(debugController
                                                          .ReleaseAndGetAddressOf())));
            debugController->EnableDebugLayer();
         }
#endif

         expectRes("create device",
                   D3D12CreateDevice(nullptr,
                                     D3D_FEATURE_LEVEL_11_0,
                                     IID_PPV_ARGS(mDevice
                                                  .ReleaseAndGetAddressOf())));

         expectRes("create DXGI factory",
                   CreateDXGIFactory1(IID_PPV_ARGS(mDXGIFactory
                                                   .ReleaseAndGetAddressOf())));

         expectRes("create fence",
                   mDevice
                   ->CreateFence(mFenceVal,
                                 D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(mFence
                                              .ReleaseAndGetAddressOf())));

         mDsvDescriptorSize = mDevice
            ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
         mRtvDescriptorSize = mDevice
            ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
         mCbvSrvUavDescriptorSize = mDevice
            ->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


         return true;
      }
      bool initMsaa()
      {
         D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msql;
         msql.Format = mBackBufferFormat;
         msql.SampleCount = 4;
         msql.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
         msql.NumQualityLevels = 0;

         expectRes("Query MSAA feature support",
                   mDevice
                   ->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                         &msql,
                                         sizeof(msql)));
         mMsaaQualityLevel = msql.NumQualityLevels;

         expectTrue("invalid quality levels",
                    mMsaaQualityLevel > 0);

         return true;
      }

      bool initCommand()
      {
         D3D12_COMMAND_QUEUE_DESC cqd = {};
         cqd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
         cqd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
         expectRes("Create command queue",
                   mDevice
                   ->CreateCommandQueue(&cqd,
                                        IID_PPV_ARGS(mCommandQueue
                                                     .ReleaseAndGetAddressOf())));

         expectRes("Create command allocator",
                   mDevice
                   ->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            IID_PPV_ARGS(mDirectCmdListAlloc
                                                         .ReleaseAndGetAddressOf())));

         expectRes("Create command list",
                   mDevice
                   ->CreateCommandList(0, // TODO: Multithreading
                                       D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       mDirectCmdListAlloc.Get(),
                                       nullptr, // TODO: PSO
                                       IID_PPV_ARGS(mCommandList
                                                    .ReleaseAndGetAddressOf())));
         mCommandList->Close(); // TODO: do I need to to this?

         return true;
      }
 };
}