#pragma once

// From libc
struct free_elem {
  size_t size;
  struct free_elem *next;
};
extern uint16_t *__brkval;
extern unsigned int __heap_start;
extern struct free_elem *__flp;

// Calculates the largest free block, and total free memory
size_t largest_free_block(uint16_t &num_fragments, size_t &free_memory)
{
  size_t max = 0;
  num_fragments = 0;
  #if defined(ARDUINO)
  #ifdef PJON_ESP
  free_memory = ESP.getFreeHeap();
  max = free_memory;
  #else
  if (__brkval == 0) free_memory = max = (uint16_t)(&max - &__heap_start);
  else {
    free_memory = (size_t)(&max - __brkval);
    // Walk through all free blocks    
    struct free_elem *free_block;
    for (free_block = __flp; free_block; free_block = free_block->next) {
      num_fragments++;
      free_memory += free_block->size + 2;
      if (free_block->size > max) max = free_block->size;
    }
  }
  #endif
  #else
  free_memory = 0;
  #endif
  return max;
}

// Return an integer percentage of the largest fragment divided by total free memory.
// (No fragmentation gives 100%, low numbers are bad.)
uint8_t get_largest_fragment_percentage() {
  uint16_t num_fragments;
  size_t free_mem;
  size_t largest_block = largest_free_block(num_fragments, free_mem);
  if (free_mem == 0) return 0;
  return (uint8_t) (100ul * (uint32_t)largest_block / (uint32_t)free_mem);
}