#include "arena.h"

#include "logging.h"
#include "math_utils.h"

// TODO default to 4-byte alignment

namespace arena {

void Arena::init(const char* name, void* start, u32 size) {
    m_name = name;
    m_start = start;
    m_capacity = size;
    reset();
}

void Arena::reset() {
    m_occupied = 0;
    mkb::memset(m_start, 0, m_capacity);
}

void* Arena::alloc_bytes(u32 size, u32 align) {
    m_occupied = ALIGN_TO(m_occupied, align);

    u32 new_occupied = m_occupied + size;
    if (new_occupied > m_capacity) {
        ABORT_MSG("[wsmod] %s arena out of memory", m_name);
    }
    void* ret = reinterpret_cast<void*>(reinterpret_cast<u32>(m_start) + m_occupied);
    m_occupied = new_occupied;
    return ret;
}

void* Arena::alloc_remaining_bytes(u32 align, u32* out_size) {
    m_occupied = ALIGN_TO(m_occupied, align);
    u32 size = m_capacity - m_occupied;
    if (out_size != nullptr) *out_size = size;
    return alloc_bytes(size, align);
}

void Arena::restore_occupied(u32 occupied) {
    ASSERT(occupied < m_capacity);
    if (occupied < m_occupied) {
        mkb::memset((void*)((u32)m_start + occupied), 0, m_occupied - occupied);
    }
    m_occupied = occupied;
}

}  // namespace arena
