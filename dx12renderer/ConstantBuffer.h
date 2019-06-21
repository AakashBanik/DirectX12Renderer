#pragma once

#include "stdafx.h"
#include "UploadBuffer.h"

namespace dmp
{
   template <typename ConstantT>
   class ConstantBuffer
   {
   public:
      ConstantBuffer() = delete;
      ConstantBuffer(ID3D12Device * withDev,
                     ID3D12DescriptorHeap * onHeap,
                     UINT numElems,
                     UINT baseOffset,
                     UINT descriptorSize) :
         mNumElems(numElems),
         mBaseOffset(baseOffset),
         mDescriptorSize(descriptorSize),
         upload(withDev, numElems)
      {
         init(withDev, onHeap);
      }

      void update(UINT i, const ConstantT & value)
      {
         upload.copyData((size_t) i, value);
      }

      void bind(ID3D12GraphicsCommandList * toList,
                ID3D12DescriptorHeap * onHeap,
                UINT toSlot,
                UINT index)
      {
         UINT i = mBaseOffset + index;
         auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(onHeap->GetGPUDescriptorHandleForHeapStart());
         handle.Offset(i, mDescriptorSize);

         toList->SetGraphicsRootDescriptorTable(toSlot, handle);
      }

   private:
      bool mValid = false;
      UINT mNumElems;
      UINT mBaseOffset;
      UINT mDescriptorSize;
      ConstantUploadBuffer<ConstantT> upload;

      void init(ID3D12Device * withDev, ID3D12DescriptorHeap * onHeap)
      {
         UINT objCBSize = (UINT) calcConstantBufferByteSize(sizeof(ConstantT));
         D3D12_GPU_VIRTUAL_ADDRESS addr = upload.getBuffer()->GetGPUVirtualAddress();

         for (UINT i = 0; i < mNumElems; ++i)
         {
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(onHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset((int) (mBaseOffset + i), mDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cvd;
            cvd.BufferLocation = addr + (i * objCBSize);
            cvd.SizeInBytes = objCBSize;

            withDev->CreateConstantBufferView(&cvd, handle);
         }
      }
   };
}