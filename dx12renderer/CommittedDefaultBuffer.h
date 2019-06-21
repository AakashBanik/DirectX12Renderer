#pragma once

#include "stdafx.h"
#include "DX12Buffer.h"

namespace dmp
{
   template <typename DataT, typename BindInfoT>
   class CommittedDefaultBuffer : public virtual DX12WriteBuffer<DataT, BindInfoT>
   {
   public:
      CommittedDefaultBuffer() = delete;
      CommittedDefaultBuffer(const CommittedDefaultBuffer &) = delete;
      CommittedDefaultBuffer & operator=(const CommittedDefaultBuffer &) = delete;

      CommittedDefaultBuffer(std::vector<DataT> & data,
                             const DX12Buffer::WriteInfo & wi,
                             ID3D12Device * dev,
                             size_t customAlignment = sizeof(DataT)) :
         mAlignment(customAlignment),
         mCPUBacking(data.size())
      {
         init(dev);
         write(0, wi, data.data(), data.size());
      }
      
      virtual size_t alignment() override
      {
         return mAlignment;
      }

      virtual size_t size() override
      {
         return mCPUBacking.size();
      }

      virtual void write(size_t offset,
                         const DX12Buffer::WriteInfo & wi, 
                         DataT * val, 
                         size_t numElems) override
      {
         for (size_t i = offset; i < numElems; ++i)
         {
            mCPUBacking[i] = val[i];
         }

         D3D12_SUBRESOURCE_DATA srd = {};
         srd.pData = mCPUBacking.data();
         srd.RowPitch = mCPUBacking.size() * mAlignment;
         srd.SlicePitch = mCPUBacking.size() * mAlignment;

         wi.commandList
            ->ResourceBarrier(1,
                              &CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
                                                                    D3D12_RESOURCE_STATE_COMMON,
                                                                    D3D12_RESOURCE_STATE_COPY_DEST));
         UpdateSubresources<1>(wi.commandList, 
                               mGPUBuffer.Get(), 
                               mUploader.Get(), 
                               0, 0, 1, &srd);
         wi.commandList
            ->ResourceBarrier(1,
                              &CD3DX12_RESOURCE_BARRIER::Transition(mGPUBuffer.Get(),
                                                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                                                    D3D12_RESOURCE_STATE_GENERIC_READ));

      }

   protected:
      ~CommittedDefaultBuffer() {}
   
      size_t mAlignment = sizeof(DataT);
      std::vector<DataT> mCPUBacking;
      Microsoft::WRL::ComPtr<ID3D12Resource> mGPUBuffer = nullptr;
      Microsoft::WRL::ComPtr<ID3D12Resource> mUploader = nullptr;
      
   private:

      void init(ID3D12Device * dev)
      {
         auto byteSize = mCPUBacking.size() * mAlignment;

         expectRes("Create Committed/Default buffer",
                   dev
                   ->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                                             D3D12_HEAP_FLAG_NONE,
                                             &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                                             D3D12_RESOURCE_STATE_COMMON,
                                             nullptr,
                                             IID_PPV_ARGS(mGPUBuffer.ReleaseAndGetAddressOf())));

         expectRes("Create Upload Buffer",
                   dev
                   ->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                             D3D12_HEAP_FLAG_NONE,
                                             &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             nullptr,
                                             IID_PPV_ARGS(mUploader.ReleaseAndGetAddressOf())));
      }
   };
}