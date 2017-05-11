#pragma once

extern void miSetTime(uint32_t utc_seconds);
extern uint32_t miGetTime();
    
extern bool miIsTimeSynced();
extern bool miWasSyncedWithin(uint32_t limit_ms);
extern uint32_t miGetLastSyncedAtMs();

extern uint8_t miGetWeekDay(uint32_t utc_s); // 1=monday
extern uint16_t miGetMinuteOfDay(uint32_t utc_s);

