#pragma once

#include "arena.h"
#include "heap.h"

//
// wsmod-wide dynamic memory model, consisting of arenas and heaps
//

namespace mem {
// Arena spanning the remaining mainloop.rel relocation data free space.
extern arena::Arena wsmod_arena;

// Heap allocated at end of remaining space in wsmod arena. This is what additional chainloaded
// mods (like Practice Mod) are loaded in and provided as their own heap.
extern heap::Heap chainload_heap;

// Arena used for per-stage allocations. It is reset each time a new stage is loaded.
// It has a fixed size, statically allocated in BSS.
extern arena::Arena stage_arena;

// Arena for per-stage-retry allocations. It is reset at the start of stobj_init(). It's
// allocated from the remainder of stage_arena.
extern arena::Arena gameplay_arena;

void init_arenas();
void init_chainload_heap();
}  // namespace mem
