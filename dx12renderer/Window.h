#pragma once

#define RendererType BasicRenderer

#include "BasicRenderer.h" // TODO
#include "Timer.h"

namespace dmp
{
   class Window
   {
   public:
      Window(HINSTANCE hInstance, int width, int height, std::wstring title);
      ~Window();

      
      LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      int run();

      static Window * getByHWND(HWND key);
      static std::unordered_map<HWND, Window*> windowMap;
   private:
      bool init();
      void updateTitle();

      int mWidth = 800;
      int mHeight = 600;
      std::wstring mTitle = L"Noonoo disapproves of win32";
      HWND mhMainWnd = nullptr;
      HINSTANCE mhAppInst = nullptr;

      std::unique_ptr<RendererType> mRenderer = nullptr;
      Timer mTimer;

      bool mMinimized = false;
      bool mMaximized = false;
      bool mResizing = false;

      bool mReady = false;
   };

}
