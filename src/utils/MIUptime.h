#pragma once

#include <platforms/MIPlatforms.h>

extern uint32_t miUptimeS,               // current uptime in seconds, maintained by miUpdateUptime()
                miLastUpdatedUptimeMs;
 
void miUpdateUptime() {
  // Update uptime
  uint32_t msElapsed = millis() - miLastUpdatedUptimeMs, sElapsed = msElapsed/1000ul;
  miUptimeS += sElapsed;
  miLastUpdatedUptimeMs += sElapsed*1000ul; 
}

uint32_t miGetUptime() { miUpdateUptime(); return miUptimeS; }

