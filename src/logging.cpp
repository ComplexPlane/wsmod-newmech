#include "logging.h"

#include <cstdarg>

namespace logging {

void assert_impl(const char* file, s32 line, bool exp) {
    if (!(exp)) {
        mkb::OSPanic(const_cast<char*>(file), line, "[wsmod] Assertion failed");
        while (true);
    }
}

[[noreturn]] void abort_impl(const char* file, int line) {
    // OSPanic doesn't return and seems to actually print to Dolphin's log before freezing
    mkb::OSPanic(const_cast<char*>(file), line, "Aborted");
    while (true) {
    }
}

[[noreturn]] void abort_impl(const char* file, int line, const char* format, ...) {
    // Hecking hope this is enough room and doesn't blow up the stack
    char msg[256] = {};

    va_list args;
    va_start(args, format);
    mkb::vsprintf(msg, const_cast<char*>(format), args);
    va_end(args);

    // OSPanic doesn't return and seems to actually print to Dolphin's log before freezing
    mkb::OSPanic(const_cast<char*>(file), line, msg);
    while (true) {
    }
}

}  // namespace logging
