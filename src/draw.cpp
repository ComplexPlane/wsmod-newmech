#include "draw.h"

#include "mkb/mkb.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "assembly.h"
#include "math_utils.h"
#include "patch.h"
#include "relutil.h"

namespace draw {

namespace {

char s_notify_msg_buf[80];
s32 s_notify_frame_counter;
mkb::GXColor s_notify_color;

}  // namespace

const mkb::GXColor WHITE = {0xff, 0xff, 0xff, 0xff};
const mkb::GXColor RED = {0xfd, 0x68, 0x75, 0xff};
const mkb::GXColor ORANGE = {0xfd, 0xac, 0x68, 0xff};
const mkb::GXColor BLUE = {0x9d, 0xe3, 0xff, 0xff};
const mkb::GXColor PINK = {0xdf, 0x7f, 0xfa, 0xff};
const mkb::GXColor PURPLE = {0xb1, 0x5a, 0xff, 0xff};
const mkb::GXColor GREEN = {0x00, 0xff, 0x00, 0xff};

void init() {
    patch::write_branch(relutil::relocate_addr(0x802aeca4),
                        reinterpret_cast<void*>(main::full_debug_text_color));
}

void debug_text_palette() {
    for (char c = 0; c != 0x80; c++) {
        s32 x = c % 16 * DEBUG_CHAR_WIDTH;
        s32 y = c / 16 * DEBUG_CHAR_WIDTH;
        mkb::draw_debugtext_char_en(x, y, c, c * 2);
    }
}

namespace {

void debug_text_buf(s32 x, s32 y, mkb::GXColor color, char* buf) {
    mkb::textdraw_reset();
    // Some good fonts that seem to be always loaded:
    // FONT32_ASC_8x16,
    // FONT32_ASC_12x12,
    // FONT32_ASC_24x24,  // Fairly big
    // FONT32_ASC_16x16P, // Doesn't support lowercase letters? P means proportional
    // FONT32_ASC_16x16,  // Doesn't support lowercase letters? Monospace
    mkb::textdraw_set_font(mkb::FONT32_ASC_8x16);
    // mkb::textdraw_set_flags(mkb::TEXTDRAW_FLAG_BORDER | mkb::TEXTDRAW_FLAG_PROPORTIONAL);

    mkb::textdraw_set_pos(x, y);
    mkb::textdraw_set_alignment(mkb::ALIGN_LOWER_RIGHT);
    mkb::textdraw_set_scale(1, 1);
    mkb::textdraw_set_mul_color(RGBA(color.r, color.g, color.b, color.a));
    mkb::textdraw_set_depth(0);
    // mkb::textdraw_set_font_style(mkb::STYLE_BOLD);
    mkb::textdraw_print(buf);
}

void debug_text_v(s32 x, s32 y, mkb::GXColor color, char* format, va_list args) {
    // Shouldn't be able to print a string to the screen longer than this
    // Be careful not to overflow! MKB2 doesn't have vsnprintf
    static char buf[80];
    mkb::vsprintf(buf, format, args);
    debug_text_buf(x, y, color, buf);
}

}  // namespace

void debug_text(s32 x, s32 y, mkb::GXColor color, char* format, ...) {
    va_list args;
    va_start(args, format);
    debug_text_v(x, y, color, format, args);
    va_end(args);
}

void disp() {
    s32 notify_len = strlen(s_notify_msg_buf);
    s32 draw_x = 640 - notify_len * DEBUG_CHAR_WIDTH - 12;
    s32 draw_y = 426;
    mkb::GXColor color = s_notify_color;

    if (s_notify_frame_counter > 40) {
        color.a = 0xff - (s_notify_frame_counter - 40) * 0xff / 20;
    }
    debug_text(draw_x, draw_y, color, s_notify_msg_buf);

    s_notify_frame_counter++;
    if (s_notify_frame_counter > 60) s_notify_frame_counter = 60;
}

void notify(mkb::GXColor color, char* format, ...) {
    va_list args;
    va_start(args, format);
    mkb::vsprintf(s_notify_msg_buf, format, args);
    va_end(args);

    s_notify_frame_counter = 0;
    s_notify_color = color;
}

}  // namespace draw
