#pragma once

#include <Arduino.h>
#include <MI_PJON/PJONModuleInterfaceSet.h>
#include <MI_PJON/PJONLink.h>

// Include HTTP support if Ethernet.h has been included before MI_PJON.h.
// The reason for not always including it is that some storage and RAM is 
// taken when including Ethernet.h even if not using it.
#ifdef ethernet_h
#include <MI/ModuleInterfaceHTTPTransfer.h>
#endif