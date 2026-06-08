#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

// Makes the GC runs as frequently as possible (stress test).
#define DEBUG_STRESS_GC

// Prints information to the console when some operation with dynamic memory occurs.
#define DEBUG_LOG_GC

#endif