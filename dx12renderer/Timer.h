#pragma once

#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::high_resolution_clock::time_point;
using Duration = std::chrono::high_resolution_clock::duration;

namespace dmp
{
   class Timer
   {
   public:
      Timer();
      ~Timer();

      float time() const;
      float deltaTime() const;

      void reset();
      void unpause();
      void pause();
      void tick();

      bool isPaused() const { return mPaused; }
   private:
      TimePoint mBaseTime;
      TimePoint mPausedTime;
      TimePoint mStoppedTime;
      TimePoint mPrevTime;
      TimePoint mCurrTime;

      Duration mDeltaTime;

      bool mPaused;

   };
}