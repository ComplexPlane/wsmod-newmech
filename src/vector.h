#pragma once

#include "arena.h"
#include "logging.h"
#include "mkb/mkb.h"

// Returns:
// -1 if a < b
//  1 if a > b
//  0 if a == b
template <typename T>
using SortFunc = int (*)(const T* a, const T* b);

template <typename T>
class Vector {
 public:
    Vector() = default;
    Vector(const Vector&) = delete;
    Vector(Vector&&) = delete;

    void alloc(arena::Arena* arena, u32 max_elems) {
        m_count = 0;
        m_capacity = max_elems;
        m_elems = arena->alloc_array<T>(max_elems);
    }

    T& operator[](u32 idx) {
        ASSERT(idx < m_count);
        return m_elems[idx];
    }

    void push(T elem) {
        ASSERT(m_count < m_capacity);
        m_elems[m_count] = elem;
        m_count++;
    }

    void pop() {
        ASSERT(m_count > 0);
        m_elems[m_count - 1] = {};
        m_count--;
    }

    u32 count() {
        return m_count;
    }

    T* data() {
        return m_elems;
    }

    void clear() {
        mkb::memset(m_elems, 0, m_count * sizeof(T));
    }

    void sort(SortFunc<T> sort_func) {
        mkb::qsort(m_elems, m_count, sizeof(T), sort_func);
    }

 private:
    T* m_elems;
    u32 m_capacity;
    u32 m_count;
};

template <typename T>
struct Optional {
    T opt;
    bool present;
};
