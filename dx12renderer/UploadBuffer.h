#pragma once

#include "stdafx.h"

namespace dmp
{
   template <typename T> 
   class UploadBuffer
   {
   protected:
      size_t mSizeofT;
   private:
      Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
      BYTE * mData;
      bool valid = false;
   public:
      UploadBuffer() = default;
      UploadBuffer(ID3D12Device * dev, size_t count)
         : mSizeofT(sizeof(T))
      {
         init(dev, count);
      }

      UploadBuffer(const UploadBuffer & rhs) = delete;
      UploadBuffer& operator=(const UploadBuffer & rhs) = delete;

      void init(ID3D12Device * dev, size_t count)
      {
         expectRes("Create the Upload Buffer",
                   dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                                                D3D12_HEAP_FLAG_NONE,
                                                &CD3DX12_RESOURCE_DESC::Buffer(mSizeofT * count),
                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                nullptr,
                                                IID_PPV_ARGS(mUploadBuffer
                                                             .ReleaseAndGetAddressOf())));

         expectRes("Map the buffer",
                   mUploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mData)));

         valid = true;
      }

      virtual ~UploadBuffer()
      {
         if (mUploadBuffer != nullptr)
         {
            mUploadBuffer->Unmap(0, nullptr);
         }
      }

      ID3D12Resource * getBuffer()
      {
         expectTrue("Not default constructed", valid);
         return mUploadBuffer.Get();
      }

      void copyData(size_t idx, const T & data)
      {
         expectTrue("Not default constructed", valid);
         memcpy(&mData[idx * mSizeofT], &data, sizeof(T));
      }
      
   };

   template <typename T> class
      ConstantUploadBuffer : public UploadBuffer<T>
   {
   public:
      ConstantUploadBuffer(ID3D12Device * dev, size_t count)
      {
         mSizeofT = calcConstantBufferByteSize(sizeof(T));
         init(dev, count);
      }
   };
}