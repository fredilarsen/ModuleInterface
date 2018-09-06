#include "platforms/MISystemDefines.h"

uint32_t miTimeS = 0,                 // current time in UTC seconds, maintained by update()
         miLastUpdatedTimeMs = 0,     // system time in ms for last update()
         miLastSyncedMs = 0;          // system time in ms() for last setTime() call from external time source
 