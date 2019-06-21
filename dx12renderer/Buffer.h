#pragma once

#include "stdafx.h"

namespace dmp
{
   template <typename BindInfoT>
   class Buffer
   {
   public:
      virtual size_t alignment() = 0;
      virtual size_t size() = 0;
      virtual void bind(const BindInfoT & bindInfo) = 0;
      virtual void unbind(const BindInfoT & bindInfo) = 0;
   protected:
      ~Buffer() {}
   };

   template <typename DataT, typename ReadInfoT, typename BindInfoT>
   class ReadBuffer : public Buffer<BindInfoT>
   {
   public:
      virtual DataT & read(size_t index, const ReadInfoT & ri) = 0; // TODO: what should this
                                                                    // signature look like?
   protected:
      ~ReadBuffer() {}
   };

   template <typename DataT, typename WriteInfoT, typename BindInfoT>
   class WriteBuffer : public Buffer<BindInfoT>
   {
   public:
      virtual void write(size_t offset, const WriteInfoT & wi, DataT * val, size_t numElems) = 0;
   protected:
      ~WriteBuffer() {}
   };
}