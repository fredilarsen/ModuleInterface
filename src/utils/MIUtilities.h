#pragma once

// Function for running at specified intervals, taking overflow into account
bool mi_interval_elapsed(uint32_t &lastRun, uint32_t intervalMs) {
  // Determine if the specified inteval has elapsed since the last execution,
  // taking overvlow / rollover into account. 
  // Ref http://playground.arduino.cc/Code/TimingRollover
  // (Do not compare timestamps -- Comparing durations is fine)
  uint32_t now = millis();
  if ((uint32_t) (now - lastRun) >= intervalMs) {
    lastRun = now;
    return true;
  }
  return false;
}

// Comparing two millisecond timestamps (taking rollover into account)
// Timestamps must be closer than 2^32/2 (a little more than 24 days)
bool mi_is_after(uint32_t A, uint32_t B){ // Returns true if A later than B
	return ((A-B) & 0x80000000) == 0; // Extracting sign bit. If negative, B>A 
}

bool mi_after_or_equal(uint32_t A, uint32_t B){
	if (A == B) return true;
	return mi_is_after(A, B);
}

// Low pass filtering. The factor must be between 0 and 1.
// This function should be called at regular intervals, depending on the specified factor.
// Example: lowpass_value = mi_lowpass(new_value, lowpass_value, 0.001f);
float mi_lowpass(float new_value, float previous_lowpass, float factor) {
  if (isfinite(new_value)) return (factor*new_value) + (1.0f-factor)*previous_lowpass;
  return previous_lowpass;
}

// Low pass filtering. The factor must be between 0 and 1.
// This function will do sampling every sample_interval milliseconds, to make sure the reaction time is predictable.
// Example: lowpass_value = mi_lowpass(new_value, lowpass_value, 0.001);
float mi_lowpass(float new_value, float previous_lowpass, float factor, uint32_t &last_lowpass_millis, const uint32_t sample_interval) {
  if (isfinite(new_value) && mi_interval_elapsed(last_lowpass_millis, sample_interval)) {
    return (factor*new_value) + (1.0f-factor)*previous_lowpass;
  }
  return previous_lowpass;
}

bool mi_compare_ignorecase(const char *a, const char *b, uint16_t len) {
  const char *pa = a, *pb = b;
  for (; pa-a < len && *pa != 0 && (*pa == *pb || tolower(*pa) == tolower(*pb)); pa++, pb++) ;
  return (pa-a == len || *pa == *pb);
}
void mi_lowercase(char *buf) { for (char *p = buf; *p != 0; p++) *p = tolower(*p); }

template<class X> X mi_max(const X &a, const X &b) { return a > b ? a : b; }
template<class X> X mi_min(const X &a, const X &b) { return a < b ? a : b; }
