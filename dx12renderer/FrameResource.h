#pragma once

#include "ConstantBuffer.h"

namespace dmp
{
   template <typename PassConstantT, typename MaterialConstantT, typename ObjectConstantT>
   struct FrameResource
   {
      FrameResource(ID3D12Device * dev, 
                    ID3D12DescriptorHeap * onHeap,
                    UINT descriptorSize,
                    UINT passCount, 
                    UINT passOffset,
                    UINT matCount,
                    UINT matOffset,
                    UINT objCount,
                    UINT objOffset) :
         passCB(std::make_unique<ConstantBuffer<PassConstantT>>(dev, 
                                                                onHeap,
                                                                passCount,
                                                                passOffset,
                                                                descriptorSize)),
         materialCB(std::make_unique<ConstantBuffer<MaterialConstantT>>(dev, 
                                                                        onHeap,
                                                                        matCount,
                                                                        matOffset,
                                                                        descriptorSize)),
         objectCB(std::make_unique<ConstantBuffer<ObjectConstantT>>(dev, 
                                                                    onHeap,
                                                                    objCount,
                                                                    objOffset,
                                                                    descriptorSize))
      {
         expectRes("Create Frame Resource Command Allocator",
                   dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(allocator
                                                            .ReleaseAndGetAddressOf())));
      }

      FrameResource(const FrameResource& rhs) = delete;
      FrameResource& operator=(const FrameResource& rhs) = delete;
      ~FrameResource() {}

      // fields
      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
      uint64_t fenceVal = 0;
      
      std::unique_ptr<ConstantBuffer<PassConstantT>> passCB = nullptr;
      std::unique_ptr<ConstantBuffer<MaterialConstantT>> materialCB = nullptr;
      std::unique_ptr<ConstantBuffer<ObjectConstantT>> objectCB = nullptr;
   };
}
