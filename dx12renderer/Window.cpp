#include "stdafx.h"
#include "Window.h"

std::unordered_map<HWND, dmp::Window*> dmp::Window::windowMap;

dmp::Window::Window(HINSTANCE hInstance, 
                    int width, 
                    int height, 
                    std::wstring title)
   : mWidth(width)
   , mHeight(height)
   , mTitle(title)
   , mhAppInst(hInstance)
{
   expectTrue("create window", init());
   mRenderer = std::make_unique<RendererType>(mhMainWnd, mWidth, mHeight);
   mReady = true;
}


dmp::Window::~Window()
{}

dmp::Window * dmp::Window::getByHWND(HWND key)
{
   auto i = windowMap.find(key);

   if (i != windowMap.end()) return i->second;

   return nullptr;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   auto w = dmp::Window::getByHWND(hwnd);
   if (w != nullptr)
   {
      return w->wndProc(hwnd, msg, wParam, lParam);
   }
   else
   {
      return DefWindowProc(hwnd, msg, wParam, lParam);
   }
}

bool dmp::Window::init()
{
   WNDCLASS wc;
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = MainWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = mhAppInst;
   wc.hIcon = LoadIcon(0, IDI_APPLICATION);
   wc.hCursor = LoadCursor(0, IDC_ARROW);
   wc.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
   wc.lpszMenuName = 0;
   wc.lpszClassName = L"MainWnd";

   if (!RegisterClass(&wc))
   {
      MessageBox(0, L"RegisterClass Failed.", 0, 0);
      return false;
   }

   // Compute window rectangle dimensions based on requested client area dimensions.
   RECT R = {0, 0, mWidth, mHeight};
   AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
   int width = R.right - R.left;
   int height = R.bottom - R.top;

   mhMainWnd = CreateWindow(L"MainWnd", mTitle.c_str(),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
   if (!mhMainWnd)
   {
      MessageBox(0, L"CreateWindow Failed.", 0, 0);
      return false;
   }

   windowMap.insert({mhMainWnd, this});

   ShowWindow(mhMainWnd, SW_SHOW);
   UpdateWindow(mhMainWnd);

   return true;
}

void dmp::Window::updateTitle()
{
   using namespace std;
   // Code computes the average frames per second, and also the 
   // average time it takes to render one frame.  These stats 
   // are appended to the window caption bar.

   static int frameCnt = 0;
   static float timeElapsed = 0.0f;
   float runtime = mTimer.time();

   frameCnt++;

   // Compute averages over one second period.
   if ((runtime - timeElapsed) >= 1.0f)
   {
      float fps = (float) frameCnt; // fps = frameCnt / 1
      float mspf = 1000.0f / fps;

      wstring fpsStr = to_wstring(fps);
      wstring mspfStr = to_wstring(mspf);
      wstring runtimeStr = to_wstring(runtime);

      wstring windowText = mTitle +
         L"   fps: " + fpsStr +
         L"   mspf: " + mspfStr +
         L"   Total Runtime: " + runtimeStr;


      SetWindowText(mhMainWnd, windowText.c_str());

      // Reset for next average.
      frameCnt = 0;
      timeElapsed += 1.0f;
   }
}

LRESULT dmp::Window::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (!mReady) return DefWindowProc(hwnd, msg, wParam, lParam);

   switch (msg)
   {
      case WM_SIZE:
         mWidth = LOWORD(lParam);
         mHeight = HIWORD(lParam);

         if (wParam == SIZE_MINIMIZED)
         {
            mTimer.pause();
            mMinimized = true;
            mMaximized = false;
         }
         else if (wParam == SIZE_MAXIMIZED)
         {
            mTimer.unpause();
            mMinimized = false;
            mMaximized = true;
            mRenderer->resize(mWidth, mHeight);
         }
         else if (wParam == SIZE_RESTORED)
         {

            // Restoring from minimized state?
            if (mMinimized)
            {
               mTimer.unpause();
               mMinimized = false;
               mRenderer->resize(mWidth, mHeight);
            }

            // Restoring from maximized state?
            else if (mMaximized)
            {
               mTimer.unpause();
               mMaximized = false;
               mRenderer->resize(mWidth, mHeight);
            }
            else if (mResizing)
            {
               // If user is dragging the resize bars, we do not resize 
               // the buffers here because as the user continuously 
               // drags the resize bars, a stream of WM_SIZE messages are
               // sent to the window, and it would be pointless (and slow)
               // to resize for each WM_SIZE message received from dragging
               // the resize bars.  So instead, we reset after the user is 
               // done resizing the window and releases the resize bars, which 
               // sends a WM_EXITSIZEMOVE message.
            }
            else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
            {
               mRenderer->resize(mWidth, mHeight);
            }
         }

         return 0;
         // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
      case WM_ENTERSIZEMOVE:
         mTimer.pause();
         mResizing = true;
         mTimer.pause();
         return 0;

         // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
         // Here we reset everything based on the new window dimensions.
      case WM_EXITSIZEMOVE:
         mTimer.unpause();
         mResizing = false;
         mTimer.unpause();
         mRenderer->resize(mWidth, mHeight);
         return 0;
         // WM_DESTROY is sent when the window is being destroyed.
      case WM_DESTROY:
         PostQuitMessage(0);
         return 0;
         // Catch this message so to prevent the window from becoming too small.
      case WM_GETMINMAXINFO:
         ((MINMAXINFO*) lParam)->ptMinTrackSize.x = 200;
         ((MINMAXINFO*) lParam)->ptMinTrackSize.y = 200;
         return 0;
      default:
         return DefWindowProc(hwnd, msg, wParam, lParam);
   }
}

int dmp::Window::run()
{
   MSG msg = {0};

   mTimer.reset();
   mTimer.unpause();

   while (msg.message != WM_QUIT)
   {
      // If there are Window messages then process them.
      if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      // Otherwise, do animation/game stuff.
      else
      {
         mTimer.tick();

         if (!mTimer.isPaused())
         {
            updateTitle();
            mRenderer->update(mTimer);
            mRenderer->draw();
         }
         else
         {
            Sleep(100);
         }
      }
   }

   return (int) msg.wParam;
}