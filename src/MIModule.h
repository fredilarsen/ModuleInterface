#pragma once

#include <platforms/MIPlatforms.h>
#include <MI_PJON/PJONModuleInterface.h>
#include <MI_PJON/PJONLink.h>
#ifdef ARDUINO
#include <MI/ModuleInterfacePersistence.h>
#endif
#include <utils/MIUtilities.h>

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