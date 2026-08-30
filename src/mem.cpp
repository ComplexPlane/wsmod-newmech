#include "mem.h"
#include "relutil.h"

namespace mem {

namespace {
u8 s_stage_arena[1024 * 16] = {};
}  // namespace

arena::Arena wsmod_arena;
arena::Arena stage_arena;
arena::Arena gameplay_arena;
heap::Heap chainload_heap;

void init_arenas() {
    // Allocate wsmod arena from available mainloop REL space
    u32 start = mkb::OSRoundUp32B(
        *reinterpret_cast<u32*>(0x8000452C));  // Set by REL loader to region after wsmod REL/BSS
    void* end_ptr = relutil::compute_mainloop_reldata_boundary(reinterpret_cast<void*>(start));
    u32 end = mkb::OSRoundDown32B(reinterpret_cast<u32>(end_ptr));
    u32 size = end - start;
    mem::wsmod_arena.init("wsmod", reinterpret_cast<void*>(start), size);

    // Init stage arena
    mem::stage_arena.init("stage", s_stage_arena, sizeof(s_stage_arena));
}

void init_chainload_heap() {
    // Allocate chainload heap from remaining wsmod arena
    // Subtract a lil more just to be safe w.r.t. arena start/end alignment
    u32 heap_size = 0;
    void* heap_start = mem::wsmod_arena.alloc_remaining_bytes(32, &heap_size);
    mem::chainload_heap.init(heap_start, heap_size);
}

}  // namespace mem
