#pragma once

#include <platforms/MIPlatforms.h>

#define MI_SECONDS_PER_DAY 86400ul
#define MI_SECONDS_YEAR_2017 1483228800ul

extern uint32_t miTimeS,              // current time in UTC seconds, maintained by update()
                miLastUpdatedTimeMs,  // system time in ms for last update()
                miLastSyncedMs;       // system time in ms() for last setTime() call from external time source
 
void miUpdateTime() {
#ifndef MI_USE_SYSTEMTIME
  // Update current time
  uint32_t msElapsed = (uint32_t)(millis() - miLastUpdatedTimeMs), sElapsed = msElapsed/1000ul;
  miTimeS += sElapsed;
  miLastUpdatedTimeMs += sElapsed*1000ul;
#endif  
}

void miSetTime(uint32_t utc_seconds) {
#ifndef MI_USE_SYSTEMTIME
  miUpdateTime();
  if (MI_SECONDS_YEAR_2017 < utc_seconds) { // Do not accept invalid time
    miTimeS = utc_seconds;
    miLastUpdatedTimeMs = miLastSyncedMs = millis();
  }
#endif  
}

uint32_t miGetTime() {
#ifdef MI_USE_SYSTEMTIME
  // Do not keep track of time, just use the system time
  return (uint32_t) time(0);
#else
  miUpdateTime(); return miTimeS;
#endif
}

bool miIsTimeSynced() {
#ifdef MI_USE_SYSTEMTIME
  return MI_SECONDS_YEAR_2017 < time(0);
#else
  miUpdateTime(); return miLastSyncedMs != 0 && MI_SECONDS_YEAR_2017 < miTimeS;
#endif
}

bool miWasSyncedWithin(uint32_t limit_ms) {
#ifdef MI_USE_SYSTEMTIME
  return miIsTimeSynced();
#else
  miUpdateTime(); return ((uint32_t)(millis() - miLastSyncedMs) <= limit_ms);
#endif  
}

uint32_t miGetLastSyncedAtMs() { 
#ifdef MI_USE_SYSTEMTIME
  return miIsTimeSynced() ? miGetTime() : 0;
#else
  miUpdateTime(); return miLastSyncedMs;
#endif  
}
  
uint8_t miGetWeekDay(uint32_t utc_s) { return ((utc_s / MI_SECONDS_PER_DAY + 3) % 7ul); } // 0=monday
uint32_t miGetSecondOfDay(uint32_t utc_s) { return utc_s % MI_SECONDS_PER_DAY; }
uint16_t miGetMinuteOfDay(uint32_t utc_s) { return miGetSecondOfDay(utc_s) / 60; }

