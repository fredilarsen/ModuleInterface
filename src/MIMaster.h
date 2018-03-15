#pragma once

#define IS_MASTER

#include <platforms/MIPlatforms.h>
#include <MI_PJON/PJONLink.h>
#include <MI_PJON/PJONModuleInterfaceSet.h>
#include <utils/MIUtilities.h>

// Include HTTP support if Ethernet.h has been included before MI_PJON.h.
// The reason for not always including it is that some storage and RAM is 
// taken when including Ethernet.h even if not using it.
#if defined(ethernet_h) || defined(MI_POSIX)
#include <MI_PJON/PJONModuleInterfaceHttpTransfer.h>
#endif

// Arduino includes the cpp files, but for others we must include them manually.
// If you want to include them in the project setup instead of through this header file,
// define MI_NO_CPP_INCLUSION before including this header.
#if defined(MI_POSIX) && !defined(MI_NO_CPP_INCLUSION)
#include <MI/ModuleVariable.cpp>
#include <MI/ModuleVariableSet.cpp>
#include <MI/ModuleInterface.cpp>
#include <utils/MITime.cpp>
#include <utils/MIUptime.cpp>
#endif
