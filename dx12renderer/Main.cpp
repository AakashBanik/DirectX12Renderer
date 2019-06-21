#include "stdafx.h"
#include "Window.h"
#include <locale>
#include <codecvt>
#include <comdef.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
   // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

   try
   {
      dmp::Window w(hInstance, 800, 800, L"Puto isn't fond of it either...");
      return w.run();
   }
   catch (dmp::InvariantViolation & e)
   {
#ifdef UNICODE
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      auto msg = converter.from_bytes(e.what());
      if (e.gotCode)
      {
         _com_error err(e.mErr); 
         msg = msg + L"\r\n\r\nError Code: " + err.ErrorMessage();
      }
#else
      auto msg = e.what();
      {
         _com_error err(e.mErr);
         msg = msg + "\r\n\r\nError Code: " + err.ErrorMessage();
      }
#endif
      MessageBox(nullptr, msg.c_str(), L"Invariant Violation!", MB_OK);
      return EXIT_FAILURE;
   }
}