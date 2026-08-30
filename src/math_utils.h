#pragma once

#include "mkb/mkb.h"

#define LEN(array) (sizeof(array) / sizeof((array)[0]))
#define ALIGN_TO(n, a) ((n) + (a - 1)) & ~(a - 1);
#define RGBA(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

namespace mathutils {

template <typename T>
inline T min(T a, T b) {
    return a < b ? a : b;
}

template <typename T>
inline T max(T a, T b) {
    return a > b ? a : b;
}

template <typename T>
inline T clamp(T x, T min_val, T max_val) {
    return min(max_val, max(min_val, x));
}

inline s16 deg_to_s16(f32 angle) {
    return static_cast<s16>(angle * 0x8000 / 180);
}

inline f32 s16_to_deg(s16 angle) {
    return static_cast<f32>(angle) * 180 / 0x8000;
}

template <typename T>
inline T pos_mod(T x, T m) {
    return (x % m + m) % m;
}

inline float lerp(float t, float a, float b) {
    return (1.f - t) * a + t * b;
}

inline float inv_lerp(float v, float a, float b) {
    return (v - a) / (b - a);
}

/* Basic vec2 utilities */

inline Vec2d vec2_add(const Vec2d& v1, const Vec2d& v2) {
    return Vec2d{v1.x + v2.x, v1.y + v2.y};
}

inline Vec2d vec2_sub(const Vec2d& v1, const Vec2d& v2) {
    return Vec2d{v1.x - v2.x, v1.y - v2.y};
}

inline Vec2d vec2_scale(f32 scale, const Vec2d& v) {
    return Vec2d{scale * v.x, scale * v.y};
}

inline f32 vec2_dot(const Vec2d& v1, const Vec2d& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

inline f32 vec2_dist_sq(const Vec2d& v1, const Vec2d& v2) {
    Vec2d delta = vec2_sub(v1, v2);
    return vec2_dot(delta, delta);
}

inline Vec2d vec2_lerp(f32 t, const Vec2d& v1, const Vec2d& v2) {
    return vec2_add(vec2_scale(1.f - t, v1), vec2_scale(t, v2));
}

/* Basic vec3 utilities */

inline Vec vec_add(const Vec& v1, const Vec& v2) {
    return Vec{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

inline Vec vec_sub(const Vec& v1, const Vec& v2) {
    return Vec{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

inline Vec vec_scale(f32 scale, const Vec& v) {
    return Vec{scale * v.x, scale * v.y, scale * v.z};
}

inline f32 vec_dot(const Vec& v1, const Vec& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline f32 vec_dist_sq(const Vec& v1, const Vec& v2) {
    Vec delta = vec_sub(v1, v2);
    return vec_dot(delta, delta);
}

inline Vec vec_lerp(f32 t, const Vec& v1, const Vec& v2) {
    return vec_add(vec_scale(1.f - t, v1), vec_scale(t, v2));
}

}  // namespace mathutils
