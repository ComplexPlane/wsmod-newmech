#pragma once

#include <type_traits>

#include "mkb/mkb.h"

// Defines a trampoline struct `name` for hooking function `s` with the override `d`.
// `d` may be a function pointer or a captureless lambda.
// Call the original hooked function via `name.chain(...)`, and install the hook
// from an init function via `HOOK_TRAMP(name)`.
#define TRAMP(name, s, d)        \
    struct {                     \
        decltype(&(s)) src;      \
        decltype(&(s)) override; \
        decltype(&(s)) chain;    \
        u32 instrs[2];           \
    } name = {.src = (&s), .override = (d)}

// Installs a trampoline defined by `TRAMP`. Must be called exactly once per trampoline,
// from a function which runs after the source function has been relocated into RAM.
#define HOOK_TRAMP(tramp)                                                                          \
    ({                                                                                             \
        decltype(&(tramp)) _tramp_ = &(tramp);                                                     \
        patch::hook_function_internal(reinterpret_cast<void*>(_tramp_->src),                       \
                                      reinterpret_cast<void*>(_tramp_->override), _tramp_->instrs, \
                                      reinterpret_cast<void**>(&_tramp_->chain));                  \
    })

namespace patch {

void clear_dc_ic_cache(void* ptr, u32 size);

// These return the overwritten word
u32 write_branch(void* ptr, void* destination);
u32 write_branch_bl(void* ptr, void* destination);
u32 write_blr(void* ptr);
u32 write_branch_main(void* ptr, void* destination, u32 branch);
u32 write_word(void* ptr, u32 data);
u32 write_nop(void* ptr);

template <typename T>
struct Tramp {
    u32 instrs[2];  // Overwritten instruction and branch to original hooked function
    T dest;         // Call this function to call the original hooked function
};

void hook_function_internal(void* function, void* dest);
void hook_function_internal(void* func, void* dest, u32* tramp_instrs, void** tramp_dest);

/**
 * Run function `dest` in place of function `func`.
 *
 * Use this version if you don't wish to call the original hooked function again.
 * `dest` may be a function pointer or a captureless lambda with a matching signature.
 */
template <typename Func, typename Dest>
void hook_function(Func func, Dest dest) {
    // Unary `+` converts a captureless lambda to its function-pointer type (and is a no-op on a
    // function pointer), so we can verify `dest`'s signature matches `func`'s at compile time.
    static_assert(std::is_same_v<Func, decltype(+dest)>,
                  "hooked function and replacement must have the same signature");
    hook_function_internal(reinterpret_cast<void*>(func), reinterpret_cast<void*>(+dest));
}

}  // namespace patch
