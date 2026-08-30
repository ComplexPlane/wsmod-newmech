#include "modlink.h"

#include "heap.h"
#include "mem.h"
#include "version.h"

namespace modlink {

namespace {

constexpr u32 MODLINK_ADDR = 0x800a9cb4;
constexpr u32 MAGIC = 0xFEEDC0DE;
const version::SemVer MODLINK_VERSION = {1, 2, 0};

ModLinkPart2 s_part2 = {};

}  // namespace

void init() {
    ModLink* link = reinterpret_cast<ModLink*>(MODLINK_ADDR);
    link->magic = MAGIC;
    link->modlink_version = MODLINK_VERSION;
    link->wsmod_version = version::WSMOD_VERSION;
    link->malloc_func = [](u32 size) { return mem::chainload_heap.alloc(size); };
    link->heap_info = &mem::chainload_heap.get_heap_info();
    link->part2 = &s_part2;
}

void set_card_work_area(void* buf) {
    s_part2.card_work_area = buf;
}

void set_savestate_func(SaveStateFunc func) {
    s_part2.savestate_func = func;
}

}  // namespace modlink
