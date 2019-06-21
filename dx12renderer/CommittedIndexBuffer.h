#pragma once

#include "stdafx.h"
#include "CommittedDefaultBuffer.h"

namespace dmp
{
   struct IndexBindInfo
   {
      ID3D12GraphicsCommandList * commandList;
   };

   template <typename IndexT>
   class CommittedIndexBuffer : public virtual CommittedDefaultBuffer<IndexT, IndexBindInfo>
   {
   public:
      CommittedIndexBuffer(std::vector<IndexT> & data,
                           const DX12Buffer::WriteInfo & wi,
                           ID3D12Device * dev) :
         CommittedDefaultBuffer(data, wi, dev, sizeof(IndexT))
      {
         if (std::is_same<IndexT, uint16_t>::value)
         {
            mIdxFormat = DXGI_FORMAT_R16_UINT;
         }
         else if (std::is_same<IndexT, uint32_t>::value)
         {
            mIdxFormat = DXGI_FORMAT_R32_UINT;
         }
         else expectTrue("IndexT must be uint32_t or uint16_t!", false);
      }

      virtual void bind(const IndexBindInfo & bi) override
      {
         D3D12_INDEX_BUFFER_VIEW ibv;
         ibv.BufferLocation = mGPUBuffer->GetGPUVirtualAddress();
         ibv.Format = mIdxFormat;
         ibv.SizeInBytes = (UINT) (alignment() * size());

         bi.commandList->IASetIndexBuffer(&ibv);

         //TODO: residency
      }

      virtual void unbind(const IndexBindInfo & bi) override
      {
         // TODO: residency
      }

      ~CommittedIndexBuffer() {}
   private:
      DXGI_FORMAT mIdxFormat;
   };

}