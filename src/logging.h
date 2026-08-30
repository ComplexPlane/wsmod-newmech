#pragma once

#include "mkb/mkb.h"

namespace logging {

// To save space and because it just makes more sense, we differentiate
// user-facing error messages from assertion failures meant for developers.
//
// abort() is for user-caused errors, with clear error messages to help resolve
// the problem.
//
// ASSERT() includes line/col number of assertion failure but not a message,
// which is more appropriate for a developer.

void assert_impl(const char* file, s32 line, bool exp);

[[noreturn]] void abort_impl(const char* file, int line);
[[noreturn]] void abort_impl(const char* file, int line, const char* format, ...);

}  // namespace logging

// Factor as much out of the macro as possible to save space
#define ASSERT(exp) (logging::assert_impl(__FILE_NAME__, __LINE__, (exp)))
#define ABORT() logging::abort_impl(__FILE_NAME__, __LINE__);
#define ABORT_MSG(format, ...) \
    (logging::abort_impl(__FILE_NAME__, __LINE__, (format)__VA_OPT__(, ) __VA_ARGS__))
