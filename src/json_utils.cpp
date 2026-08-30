#include "json_utils.h"

#include "math_utils.h"

namespace json {

bool eq(sj_Value value, const char* s) {
    size_t len = value.end - value.start;
    return mkb::strlen(const_cast<char*>(s)) == len &&
           mkb::memcmp(value.start, const_cast<char*>(s), len) == 0;
}

u32 parse_array_len(sj_Reader reader, sj_Value array) {
    sj_Value elem = {};
    u32 len = 0;
    while (sj_iter_array(&reader, array, &elem)) {
        len++;
    }
    return len;
}

s32 parse_int(sj_Value value) {
    if (value.type != SJ_NUMBER) return 0;
    return mkb::atoi(value.start);  // Hopefully doesn't overflow!
}

f32 parse_float(sj_Value value) {
    if (value.type != SJ_NUMBER) return 0.f;
    return (f32)mkb::atof(value.start);
}

bool parse_bool(sj_Value value) {
    return eq(value, "true");
}

Vec parse_vec(sj_Reader* reader, sj_Value array) {
    Vec v = {};
    sj_Value elem = {};

    sj_iter_array(reader, array, &elem);
    v.x = parse_float(elem);
    sj_iter_array(reader, array, &elem);
    v.y = parse_float(elem);
    sj_iter_array(reader, array, &elem);
    v.z = parse_float(elem);

    return v;
}

S16Vec parse_rot(sj_Reader* reader, sj_Value array) {
    Vec v = parse_vec(reader, array);
    return {mathutils::deg_to_s16(v.x), mathutils::deg_to_s16(v.y), mathutils::deg_to_s16(v.z)};
}

u32 parse_enum(sj_Value s, const char** options, u32 option_count) {
    for (u32 i = 0; i < option_count; i++) {
        if (eq(s, options[i])) {
            return i;
        }
    }
    return 0;
}

}  // namespace json
