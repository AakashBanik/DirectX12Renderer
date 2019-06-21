#include "stdafx.h"
#include "Timer.h"


dmp::Timer::Timer()
   : mBaseTime(Clock::now()),
   mPausedTime(mBaseTime),
   mStoppedTime(),
   mPrevTime(),
   mCurrTime(),
   mPaused(true)
{}


dmp::Timer::~Timer()
{}

float dmp::Timer::time() const
{
   using asFloat = std::chrono::duration<float, std::chrono::seconds::period>;

   // "effective now"
   TimePoint now;

   if (mPaused)
   {
      // If we're paused, then we consider now to be the time we paused
      now = mStoppedTime;
   }
   else
   {
      // If the clock is running, then now is the actual now
      now = mCurrTime;
   }

   // now is now set to be the base time plus the delta of the previous effective now minus how long we spent paused
   //now = mBaseTime + (now - mPausedTime);
   //auto final = now - mBaseTime;

   // The final elapsed time is the difference between the effective "now" and how long we spent paused.
   // let: epoch = [some huge number]
   //      now = epoch + 10
   //      paused = epoch + 2
   //      then final = (epoch + 10) - (epoch + 2) = 8 time units spent "running"
   auto final = now - mPausedTime;

   return std::chrono::duration_cast<asFloat>(final).count();
}

float dmp::Timer::deltaTime() const
{
   using asFloat = std::chrono::duration<float>;
   return std::chrono::duration_cast<asFloat>(mDeltaTime).count();
}

void dmp::Timer::reset()
{
   mBaseTime = Clock::now();
   mPrevTime = mBaseTime;
   mPausedTime = mBaseTime;
   mCurrTime = mBaseTime;
   mStoppedTime = mBaseTime;
   mPaused = true;
}

void dmp::Timer::unpause()
{
   if (mPaused)
   {
      auto now = Clock::now();
      mPausedTime = mPausedTime + (now - mStoppedTime);
      mPrevTime = now;
      mStoppedTime = TimePoint();
      mPaused = false;
   }
}

void dmp::Timer::pause()
{
   if (!mPaused)
   {
      mStoppedTime = Clock::now();
      mPaused = true;
   }
}

void dmp::Timer::tick()
{
   if (mPaused)
   {
      mDeltaTime = Duration(0);
      return;
   }

   auto now = Clock::now();
   mPrevTime = mCurrTime;
   mCurrTime = now;
   mDeltaTime = mCurrTime - mPrevTime;

   if (mDeltaTime < Duration(0)) mDeltaTime = Duration(0);
}
