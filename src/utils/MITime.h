#pragma once

#include <platforms/MIPlatforms.h>

#define MI_SECONDS_PER_DAY 86400ul
#define MI_SECONDS_YEAR_2017 1483228800ul

extern uint32_t miTimeS,              // current time in UTC seconds, maintained by update()
                miLastUpdatedTimeMs,  // system time in ms for last update()
                miLastSyncedMs;       // system time in ms() for last setTime() call from external time source
 

 struct miTime {

  static void Update() {
  #ifndef MI_USE_SYSTEMTIME
    // Update current time
    uint32_t msElapsed = (uint32_t)(millis() - miLastUpdatedTimeMs), sElapsed = msElapsed/1000ul;
    miTimeS += sElapsed;
    miLastUpdatedTimeMs += sElapsed*1000ul;
  #endif  
  }

  static void Set(uint32_t utc_seconds) {
  #ifndef MI_USE_SYSTEMTIME
    miTime::Update();
    if (MI_SECONDS_YEAR_2017 < utc_seconds) { // Do not accept invalid time
      miTimeS = utc_seconds;
      miLastUpdatedTimeMs = miLastSyncedMs = millis();
    }
  #endif  
  }

  static uint32_t Get() {
  #ifdef MI_USE_SYSTEMTIME
    // Do not keep track of time, just use the system time
    return (uint32_t) time(0);
  #else
    miTime::Update(); return miTimeS;
  #endif
  }

  static bool IsSynced() {
  #ifdef MI_USE_SYSTEMTIME
    return MI_SECONDS_YEAR_2017 < time(0);
  #else
    miTime::Update(); return miLastSyncedMs != 0 && MI_SECONDS_YEAR_2017 < miTimeS;
  #endif
  }

  static bool WasSyncedWithin(uint32_t limit_ms) {
  #ifdef MI_USE_SYSTEMTIME
    return miTime::IsSynced();
  #else
    miTime::Update(); return (IsSynced() && (uint32_t)(millis() - miLastSyncedMs) <= limit_ms);
  #endif  
  }

  static uint32_t GetLastSyncedAtMs() { 
  #ifdef MI_USE_SYSTEMTIME
    return miTime::IsSynced() ? miTime::Get() : 0;
  #else
    miTime::Update(); return miLastSyncedMs;
  #endif  
  }
    
  static uint8_t GetWeekDay(uint32_t utc_s) { return ((utc_s / MI_SECONDS_PER_DAY + 3) % 7ul); } // 0=monday
  static uint32_t GetSecondOfDay(uint32_t utc_s) { return utc_s % MI_SECONDS_PER_DAY; }
  static uint16_t GetMinuteOfDay(uint32_t utc_s) { return miTime::GetSecondOfDay(utc_s) / 60; }
 };
