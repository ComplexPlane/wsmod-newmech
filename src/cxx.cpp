#include "mkb/mkb.h"
#include <cstddef>

#include "heap.h"
#include "logging.h"

// These operators are banned for the time being. We need tight control over where allocations
// occur.

void* operator new(u32 size) { ABORT(); }

void* operator new[](u32 size) { ABORT(); }

void operator delete(void* ptr) { ABORT(); }

void operator delete[](void* ptr) { ABORT(); }

void operator delete(void* ptr, u32 size) { ABORT(); }

void operator delete[](void* ptr, u32 size) { ABORT(); }

namespace std {

// Helpers for exception objects in <functional>
void __throw_bad_function_call() { ABORT(); }

}  // namespace std
