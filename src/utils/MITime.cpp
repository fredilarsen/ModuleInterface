#include <Arduino.h>
#include "MITime.h"

#define MI_SECONDS_PER_DAY 86400ul
#define MI_SECONDS_YEAR_2017 1483228800ul;

uint32_t miTimeS = 0,                 // current time in UTC seconds, maintained by update()
         miLastUpdatedTimeMs = 0,    // system time in ms for last update()
         miLastSyncedMs = 0;          // system time in ms() for last setTime() call from external time source
 
void miUpdateTime() {
  // Update current time
  uint32_t msElapsed = millis() - miLastUpdatedTimeMs, sElapsed = msElapsed/1000ul;
  miTimeS += sElapsed;
  miLastUpdatedTimeMs += sElapsed*1000ul;
}    

void miSetTime(uint32_t utc_seconds) {
    miUpdateTime();
    miTimeS = utc_seconds;
    miLastUpdatedTimeMs = miLastSyncedMs = millis();
}
  
uint32_t miGetTime() { miUpdateTime(); return miTimeS; }
  
bool miIsTimeSynced() { miUpdateTime(); return miLastSyncedMs != 0 && 1483228800ul < miTimeS; }
bool miWasSyncedWithin(uint32_t limit_ms) { miUpdateTime(); return ((uint32_t)(millis() - miLastSyncedMs) <= limit_ms); }
uint32_t miGetLastSyncedAtMs() { miUpdateTime(); return miLastSyncedMs; }
  
uint8_t miGetWeekDay(uint32_t utc_s) { return ((utc_s / MI_SECONDS_PER_DAY + 3) % 7ul) + 1; } // 1=monday
uint16_t miGetMinuteOfDay(uint32_t utc_s) { return utc_s % MI_SECONDS_PER_DAY; }
  