#pragma once

// Function for running at specified intervals, taking overflow into account
bool mi_interval_elapsed(unsigned long &lastRun, unsigned long intervalMs) {
  // Determine if the specified inteval has elapsed since the last execution,
  // taking overvlow / rollover into account. 
  // Ref http://playground.arduino.cc/Code/TimingRollover
  // (Do not compare timestamps -- Comparing durations is fine)
  unsigned long now = millis();
  if ((unsigned long) (now - lastRun) >= intervalMs) {
    lastRun = now;
    return true;
  }
  return false;
}

// Comparing two millisecond timestamps (taking rollover into account)
// Timestamps must be closer than 2^32/2 (a little more than 24 days)
bool mi_is_after(unsigned long A, unsigned long B){ // Returns true if A later than B
	return ((A-B) & 0x80000000) == 0; // Extracting sign bit. If negative, B>A 
}

bool mi_after_or_equal(unsigned long A, unsigned long B){
	if (A == B) return true;
	return mi_is_after(A, B);
}
