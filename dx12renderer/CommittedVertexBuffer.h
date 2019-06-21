#pragma once

#include "stdafx.h"
#include "CommittedDefaultBuffer.h"

namespace dmp
{
   struct VertexBindInfo
   {
      ID3D12GraphicsCommandList * commandList;
      size_t startSlot;
      // TODO: need to support multiple views?
   };

   template <typename VertexT>
   class CommittedVertexBuffer : public virtual CommittedDefaultBuffer<VertexT, VertexBindInfo>
   {
   public:
      CommittedVertexBuffer(std::vector<VertexT> & data,
                            const DX12Buffer::WriteInfo & wi,
                            ID3D12Device * dev,
                            size_t customAlignment = sizeof(VertexT)) :
         CommittedDefaultBuffer(data, wi, dev, customAlignment)
      {}

      virtual void bind(const VertexBindInfo & bi) override
      {
         D3D12_VERTEX_BUFFER_VIEW vbv;
         vbv.BufferLocation = mGPUBuffer->GetGPUVirtualAddress();
         vbv.StrideInBytes = (UINT) alignment();
         vbv.SizeInBytes = (UINT) (alignment() * size());

         bi.commandList->IASetVertexBuffers((UINT) bi.startSlot, 1, &vbv);

         //TODO: residency
      }

      virtual void unbind(const VertexBindInfo & bi) override
      {
         // TODO: residency
      }

      ~CommittedVertexBuffer() {}
   };
}