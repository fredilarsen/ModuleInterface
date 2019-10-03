#pragma once

#ifndef PJON_MAX_PACKETS
  #define PJON_MAX_PACKETS 0
#endif
#ifndef PJON_PACKET_MAX_LENGTH
  #define PJON_PACKET_MAX_LENGTH 250
#endif
// Increase SWBB timeout to handle long packets
#ifndef SWBB_RESPONSE_TIMEOUT
  #define SWBB_RESPONSE_TIMEOUT 2000
#endif

#include <PJON.h>
#include "platforms/MISystemDefines.h"

#if defined(PJON_ESP) || defined(MI_POSIX)
#define F(x) (x)
#endif

#ifdef MI_POSIX
	// Include PJON TCPHelper class for connecting to web server
	#include <interfaces/LINUX/TCPHelper_POSIX.h>

	#define EthernetClient TCPHelperClient
	#define Client TCPHelperClient
#else
	#ifdef PJON_ESP
		#define isfinite(x) std::isfinite(x)
		#if defined(ESP32)
		  #include <WiFi.h>
		  #include <math.h>	
		#endif		
	#endif
#endif

