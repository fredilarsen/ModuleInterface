#include <utils/MIUptime.h>

uint32_t miUptimeS = 0,               // current uptime in seconds, maintained by miUpdateUptime()
         miLastUpdatedUptimeMs = 0;
 
void miUpdateUptime() {
  // Update uptime
  uint32_t msElapsed = millis() - miLastUpdatedUptimeMs, sElapsed = msElapsed/1000ul;
  miUptimeS += sElapsed;
  miLastUpdatedUptimeMs += sElapsed*1000ul; 
}    

uint32_t miGetUptime() { miUpdateUptime(); return miUptimeS; }    
