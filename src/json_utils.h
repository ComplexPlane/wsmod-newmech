#pragma once

extern "C" {
#include <sj.h>
}

#include "logging.h"
#include "mkb/mkb.h"

namespace json {

template <typename T>
void* read_file(T allocator, const char* path, u32* out_size) {
    mkb::DVDFileInfo dvd_file_info = {};
    if (!mkb::DVDOpen(const_cast<char*>(path), &dvd_file_info)) {
        return nullptr;
    }

    // Round up to a multiple of 32, necessary for DVDReadAsyncPrio
    u32 buf_size = mkb::OSRoundUp32B(dvd_file_info.length);
    void* buf = allocator(buf_size);
    ASSERT(buf != nullptr);

    u32 read_size =
        mkb::read_entire_file_using_dvdread_prio_async(&dvd_file_info, buf, buf_size, 0);
    ASSERT(read_size > 0);

    mkb::DVDClose(&dvd_file_info);

    if (out_size != nullptr) *out_size = read_size;
    return buf;
}

bool eq(sj_Value value, const char* s);
u32 parse_array_len(sj_Reader reader, sj_Value array);
s32 parse_int(sj_Value value);
f32 parse_float(sj_Value value);
bool parse_bool(sj_Value value);
Vec parse_vec(sj_Reader* reader, sj_Value array);
S16Vec parse_rot(sj_Reader* reader, sj_Value array);
u32 parse_enum(sj_Value s, const char** options, u32 option_count);

}  // namespace json
