#pragma once

#include "stdafx.h"
#include "CommittedVertexBuffer.h"
#include "CommittedIndexBuffer.h"
#include "DX12Buffer.h"

namespace dmp
{
   struct SubMesh
   {
      size_t indexCount = 0;
      size_t startIndexOffset = 0;
      size_t startVertexOffset = 0;
      size_t matIndex = 0;
   };

   template <typename VertexT>
   class MeshData
   {
   public:
      MeshData(const std::vector<VertexT> & verts, size_t matIndex) :
         mVerts(verts), mIndexed(false), mMatIndex(matIndex) {}
      MeshData(const std::vector<VertexT> & verts, const std::vector<uint16_t> i16, size_t matIndex) :
         mVerts(verts), mI16(i16), mIndexed(true), mMatIndex(matIndex) {}
      MeshData(const std::vector<VertexT> & verts, const std::vector<uint32_t> i32, size_t matIndex) :
         mVerts(verts), mI32(i32), mIndexed(true), mMatIndex(matIndex) {}

      bool isIndexed() const
      {
         expectTrue("Non-indexed vertices are currently not supported",
                    mIndexed);
         return mIndexed;
      }


      const std::vector<VertexT> & getVerts()
      {
         return mVerts;
      }

      const std::vector<uint16_t> & getI16()
      {
         expectTrue("vertex is indexed", mIndexed);

         if (mI16.empty())
         {
            for (const auto & curr : mI32)
            {
               mI16.push_back(static_cast<uint16_t>(curr));
            }
         }

         return mI16;
      }

      const std::vector<uint32_t> & getI32()
      {
         expectTrue("vertex is indexed", mIndexed);

         if (mI32.empty())
         {
            for (const auto & curr : mI16)
            {
               mI32.push_back(static_cast<uint32_t>(curr));
            }
         }

         return mI32;
      }

      size_t getMatIndex() { return mMatIndex; }

   private:
      bool mIndexed = false;
      std::vector<VertexT> mVerts;
      std::vector<uint16_t> mI16;
      std::vector<uint32_t> mI32;
      size_t mMatIndex;
   };

   template <typename VertexT, typename IndexT>
   class MeshBuffer
   {
   public:
      MeshBuffer() = default;

      MeshBuffer(const std::vector<MeshData<VertexT>> & verts,
                 ID3D12Device * dev,
                 ID3D12GraphicsCommandList * clist)
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

         init(verts, dev, clist);
      }

      ~MeshBuffer() {}
      
      

      CommittedVertexBuffer<VertexT> * vertexBuffer() { return mVertexBuffer.get(); }
      CommittedIndexBuffer<IndexT>  * indexBuffer() { return mIndexBuffer.get(); }

   private:
      void init(const std::vector<MeshData<VertexT>> & verts, 
                ID3D12Device * dev,
                ID3D12GraphicsCommandList * clist)
      {
         size_t indexOffset = 0;
         size_t vertexOffset = 0;

         for (MeshData<VertexT> curr : verts)
         {

            SubMesh sm;
            sm.indexCount = curr.getI16().size();
            sm.startIndexOffset = indexOffset;
            sm.startVertexOffset = vertexOffset;
            sm.matIndex = curr.getMatIndex();
         
            mSubMeshes.push_back(sm);

            indexOffset = indexOffset + curr.getI16().size();
            vertexOffset = vertexOffset + curr.getVerts().size();
         }

         std::vector<VertexT> allVerts(0);
         
         std::vector<IndexT> allIdxs(0);
         

         for (MeshData<VertexT> curr : verts)
         {
            allVerts.insert(allVerts.end(), curr.getVerts().begin(), curr.getVerts().end());
            if (std::is_same<IndexT, uint16_t>::value)
            {
               allIdxs.insert(allIdxs.end(), curr.getI16().begin(), curr.getI16().end());
            }
            else if (std::is_same<IndexT, uint32_t>::value)
            {
               allIdxs.insert(allIdxs.end(), curr.getI32().begin(), curr.getI32().end());
            }
         }

         DX12Buffer::WriteInfo wi;
         wi.commandList = clist;
         mVertexBuffer = std::make_unique<CommittedVertexBuffer<VertexT>>(allVerts, wi, dev);
         mIndexBuffer = std::make_unique<CommittedIndexBuffer<IndexT>>(allIdxs, wi, dev);
      }

      bool mValid = false;

      std::unique_ptr<CommittedIndexBuffer<IndexT>> mIndexBuffer = nullptr;

      std::unique_ptr<CommittedVertexBuffer<VertexT>> mVertexBuffer = nullptr;

      DXGI_FORMAT mIdxFormat;

      public:
         std::vector<SubMesh> mSubMeshes;
   };
}