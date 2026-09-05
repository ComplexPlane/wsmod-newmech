#pragma once

#include "mkb/mkb.h"

namespace draw {

inline constexpr u32 DEBUG_CHAR_WIDTH = 0xc;

inline constexpr u32 SCREEN_WIDTH = 640;
inline constexpr u32 SCREEN_HEIGHT = 480;

inline constexpr mkb::GXColor WHITE = {0xff, 0xff, 0xff, 0xff};
inline constexpr mkb::GXColor BLACK = {0x00, 0x00, 0x00, 0xff};
inline constexpr mkb::GXColor RED = {0xfd, 0x68, 0x75, 0xff};
inline constexpr mkb::GXColor ORANGE = {0xfd, 0xac, 0x68, 0xff};
inline constexpr mkb::GXColor BLUE = {0x9d, 0xe3, 0xff, 0xff};
inline constexpr mkb::GXColor PINK = {0xdf, 0x7f, 0xfa, 0xff};
inline constexpr mkb::GXColor PURPLE = {0xb1, 0x5a, 0xff, 0xff};
inline constexpr mkb::GXColor GREEN = {0x00, 0xff, 0x00, 0xff};

// Parameters for directly drawing a textured quad to the screen.
//
// Coordinates are in f32 screen space pixels:
// X: [0.f, 640.f]
// Y: [0.f, 480.f]
// The coordinate system is the same across 4:3 and 16:9, though in 16:9 textures are "unstretched"
// about `widescreen_x`.
struct TextureRequest {
    // Texture to draw, obtainable from e.g. `mkb::TplBuffer.texobjs`
    mkb::GXTexObj* texobj;
    // Position in screen coordinates (pixels). Rotation is applied in screen space about this
    // point
    Vec2d pos;
    // Rotation (about `pos` in screen space)
    s16 rot;
    // Width and height in screen space pixels
    Vec2d size;
    // Z position, used for stack order
    f32 depth;
    // Screen space X position about which to unstretch texture in widescreen.
    // Another way to think about it: whichever part of your texture is at this X position will not
    // not move.
    f32 widescreen_x = (f32)SCREEN_WIDTH / 2.f;

    // Pivot point UV coordinate. This point in the texture is positioned at `pos`
    Vec2d pivot_uv = {0.5f, 0.5f};
    // If you want to draw a subregion of a texture, you can override the texture UV coordinate
    // range.
    Vec2d min_uv = {0.f, 0.f};
    Vec2d max_uv = {1.f, 1.f};

    // Multiplied to each pixel's color. Specify alpha here as well
    GXColor mul_color = {0xff, 0xff, 0xff, 0xff};
    // Added to each pixel's color
    GXColor add_color = {0x00, 0x00, 0x00, 0x00};
};

// Call once during mod initialization
void init();

// Call once per frame in the mkb 2d drawing hook
void disp();

// Call once per frame in the mkb 2d drawing hook before all other disp functions of other things
void predraw();

/*
 * Functions which draw immediately
 */

void rect(float x1, float y1, float x2, float y2, mkb::GXColor color);
void debug_text(s32 x, s32 y, mkb::GXColor color, char* format, ...);
void texture(TextureRequest* req);

/*
 * Functions which cause drawing during disp() and don't necessarily need to be called each frame
 */

// Show a notification in the bottom-right of the screen which fades out after a short period
void notify(mkb::GXColor color, char* format, ...);

}  // namespace draw
