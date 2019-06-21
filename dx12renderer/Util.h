#pragma once

/*
#ifndef expect
#define expectTrue(msg, e) \
{ \
   assert((e) && "Truth Assertion Failed:" msg); \
}
#else
#error expectTrue already defined!
#endif

#ifndef expect
#define expectRes(msg, e) \
{ \
   HRESULT res = (e); \
   assert(SUCCEEDED(res) && "Result Assertion Failed:" msg); \
}
#else
#error expectRes already defined!
#endif
*/

namespace dmp
{
   static auto doNothing = []() {};

   static const UINT SWAP_CHAIN_BUFFER_COUNT = 2;
   static const UINT FRAME_RESOURCES_COUNT = 3;

   class InvariantViolation : public std::exception
   {
   public:
      InvariantViolation(std::string msg, const char * file, int line)
      {
         mMsg = std::string(msg + "\r\n\r\nIn File: " + file + "\r\n\r\nAt Line: " + std::to_string(line));
      }

      InvariantViolation(std::string msg, const char * file, int line, HRESULT res)
      {
         mMsg = std::string(msg + "\r\n\r\nIn File: " + file + "\r\n\r\nAt Line: " + std::to_string(line));
         mErr = res;
         gotCode = true;
      }

      const char * what() const
      {
         return mMsg.c_str();
      }

      bool gotCode = false;
      std::string mMsg;
      HRESULT mErr;
   };

   inline size_t calcConstantBufferByteSize(size_t unaligned)
   {
      return (unaligned + 255) & ~255;
   }
}

#ifndef expect
#define expectTrue(_msg, _e) \
{ \
   if(!(_e)) throw dmp::InvariantViolation("Truth Assertion Failed: " _msg, __FILE__, __LINE__); \
}
#else
#error expectTrue already defined!
#endif

#ifndef expect
#define expectRes(_msg, _e) \
{ \
   HRESULT _res = (_e); \
   if(!SUCCEEDED(_res)) throw dmp::InvariantViolation("Result Assertion Failed: " _msg, __FILE__, __LINE__, _res); \
}
#else
#error expectRes already defined!
#endif