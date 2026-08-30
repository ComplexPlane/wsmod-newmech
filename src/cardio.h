#pragma once

#include "arena.h"
#include "mkb/mkb.h"

namespace cardio {

enum class Slot {
    A,
    B,
};

void init(arena::Arena* arena);
void tick();

// Caller gets an arena-allocated buffer containing file
// Synchronous at the moment. Also, do not call while write_file() is running!
mkb::CARDResult read_file(arena::Arena* arena, const char* file_name, Slot slot, void** buf);

// Writes asynchronously
void write_file(const char* file_name, Slot slot, const void* buf, u32 buf_size,
                void (*callback)(mkb::CARDResult));

}  // namespace cardio
