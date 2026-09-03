#pragma once

#include "arena.h"
#include "logging.h"
#include "mkb/mkb.h"

namespace cnt {

// Returns:
// -1 if a < b
//  1 if a > b
//  0 if a == b
template <typename T>
using SortFunc = int (*)(const T* a, const T* b);

// Array is a fixed-sized "slice" of elements.
// It does not "own" its memory buffer - its allocation is managed independently, in e.g.
// statically-allocated buffers or arenas.
template <typename T>
class Array {
 public:
    Array() = default;

    explicit Array(arena::Arena* arena, u32 count)
        : m_elems{arena->alloc_array<T>(count)}, m_count{count} {
    }
    explicit constexpr Array(T* ptr, u32 count) : m_elems{ptr}, m_count{count} {
    }

    void alloc(arena::Arena* arena, u32 count) {
        m_elems = arena->alloc_array<T>(count);
        m_count = count;
    }

    T& operator[](u32 idx) {
        ASSERT(idx < m_count);
        return m_elems[idx];
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
    T* m_elems = nullptr;
    u32 m_count = 0;
};

// Vector is a variable-sized array of elements.
// Its maximum capacity is allocated upfront at construction, either from static buffers or
// arenas. It does not "own" its memory buffer - its allocation is managed independently, in
// e.g. statically-allocated buffers or arenas.
template <typename T>
class Vector {
 public:
    Vector() = default;

    // Copying a vector doesn't make sense without cloning backing storage
    Vector(const Vector&) = delete;
    Vector& operator=(const Vector&) = delete;

    // Moving is OK though
    Vector(Vector&&) = default;
    Vector& operator=(Vector&&) = default;

    explicit constexpr Vector(T* ptr, u32 max_elems)
        : m_elems{ptr}, m_capacity{max_elems}, m_count{0} {
    }
    explicit Vector(arena::Arena* arena, u32 max_elems)
        : m_elems{arena->alloc_array<T>(max_elems)}, m_capacity{max_elems}, m_count{0} {
    }

    void alloc(arena::Arena* arena, u32 count) {
        m_elems = arena->alloc_array<T>(count);
        m_capacity = count;
        m_count = 0;
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

    T* push_zeroed() {
        ASSERT(m_count < m_capacity);
        T* ret = &m_elems[m_count];
        m_count++;
        return ret;
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

    Array<T> as_array() {
        return Array(m_elems, m_count);
    }

 private:
    T* m_elems = nullptr;
    u32 m_capacity = 0;
    u32 m_count = 0;
};

template <typename T>
struct Option {
    T opt;
    bool present;

    Option() = default;
};

template <typename T>
Option<T> some(T value) {
    return Option<T>{.opt = value, .present = true};
}

template <typename T>
Option<T> none() {
    Option<T> opt;
    return opt;
}

}  // namespace cnt
