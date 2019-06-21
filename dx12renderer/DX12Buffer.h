#pragma once

#include "stdafx.h"
#include "Buffer.h"

namespace dmp
{
   namespace DX12Buffer
   {
      struct ReadInfo
      {

      };

      struct WriteInfo
      {
         ID3D12GraphicsCommandList * commandList;
      };
   }

   template <typename DataT, typename BindInfoT>
   using DX12ReadBuffer = ReadBuffer<DataT, DX12Buffer::ReadInfo, BindInfoT>;

   template <typename DataT, typename BindInfoT>
   using DX12WriteBuffer = WriteBuffer<DataT, DX12Buffer::WriteInfo, BindInfoT>;
}