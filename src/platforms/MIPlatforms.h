#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif

#if defined(_WIN32) || defined(WIN32) || defined(LINUX) || defined(RPI)
#define MI_POSIX
#endif

#ifdef MI_POSIX
	#define IS_MASTER
	#define ARDUINOJSON_ENABLE_PROGMEM 0
	typedef std::string String;

	void mi_dprint(const char *s) { printf("%s", s); }
	void mi_dprint(const int x) { printf("%d", x); }
	void mi_dprintln(const char *s) { printf("%s\n", s); }
	void mi_dprintln(const int x) { printf("%d\n", x); }

	#define DPRINTLN(x) mi_dprintln(x)
	#define DPRINT(x) mi_dprint(x)

	#include <math.h>
	
	#ifndef min
		#define min(x, y) (x <= y ? x : y)
	#endif
	
	#ifndef max
		#define max(x, y) (x >= y ? x : y)
	#endif
	
	#ifndef _itoa
	#define _itoa(x,y,z) sprintf(y, "%d", (int)x)
	#endif
	
	#define PROGMEM  
	#define pgm_read_byte(c) *(const char *)(c)	
	#define EthernetClient TCPHelperClient
	#define Client TCPHelperClient	
#endif
