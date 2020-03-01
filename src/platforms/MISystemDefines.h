#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif

#if defined(_WIN32) || defined(WIN32) || defined(LINUX) || defined(RPI)
#define MI_POSIX
#endif

#ifdef MI_POSIX
  #include <math.h>
  #include <string>

	#define ARDUINOJSON_ENABLE_PROGMEM 0
	typedef std::string String;

	void mi_dprint(const char *s) { printf("%s", s); }
	void mi_dprint(const int x) { printf("%d", x); }
	void mi_dprintln(const char *s) { printf("%s\n", s); }
	void mi_dprintln(const int x) { printf("%d\n", x); }

	#define DPRINTLN(x) mi_dprintln(x)
	#define DPRINT(x) mi_dprint(x)
	
	#ifndef _itoa
	#define _itoa(x,y,z) sprintf(y, "%d", (int)x)
	#endif
	
	#define PROGMEM  
	#define pgm_read_byte(c) *(const char *)(c)
#else
	#define DPRINTLN(x) Serial.println(x)
	#define DPRINT(x) Serial.print(x)
	#define _itoa(x,y,z) itoa(x,y,z)
#endif

#ifndef MI_min
  #define MI_min(x, y) (x <= y ? x : y)
#endif

#ifndef MI_max
  #define MI_max(x, y) (x >= y ? x : y)
#endif

