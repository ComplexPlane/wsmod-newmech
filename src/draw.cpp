#include "draw.h"

#include <cstdarg>
#include "logging.h"
#include "mkb/mkb.h"

#include "assembly.h"
#include "math_utils.h"
#include "patch.h"
#include "relutil.h"

namespace draw {

namespace {

char s_notify_msg_buf[80];
s32 s_notify_frame_counter;
mkb::GXColor s_notify_color;

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
    s32 notify_len = mkb::strlen(s_notify_msg_buf);
    s32 draw_x = SCREEN_WIDTH - notify_len * DEBUG_CHAR_WIDTH - 12;
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

void texture(TextureRequest* req) {
    ASSERT(req->texobj != nullptr);

    // Compute vertex position transform
    mkb::mtxa_from_translate_xyz(req->pos.x, req->pos.y, req->depth);
    mkb::mtxa_rotate_z(-req->rot);
    mkb::mtxa_scale_xyz(req->size.x, req->size.y, 1);
    mkb::mtxa_translate_xyz(-req->pivot_uv.x, -req->pivot_uv.y, 0);

    // Compute vertex positions
    Vec pos_top_left = {0, 0, 0};
    Vec pos_top_right = {1, 0, 0};
    Vec pos_bottom_right = {1, 1, 0};
    Vec pos_bottom_left = {0, 1, 0};
    mkb::mtxa_tf_point(&pos_top_left, &pos_top_left);
    mkb::mtxa_tf_point(&pos_top_right, &pos_top_right);
    mkb::mtxa_tf_point(&pos_bottom_right, &pos_bottom_right);
    mkb::mtxa_tf_point(&pos_bottom_left, &pos_bottom_left);

    // Compute vertex UVs
    Vec2d uv_top_left = req->min_uv;
    Vec2d uv_top_right = {req->max_uv.x, req->min_uv.y};
    Vec2d uv_bottom_right = req->max_uv;
    Vec2d uv_bottom_left = {req->min_uv.x, req->max_uv.y};

    // Set GPU texture/widescreen scale/color
    mkb::GXLoadTexObj_cached(req->texobj, mkb::GX_TEXMAP0);
    mkb::set_ui_widescreen_scale_mtx(req->widescreen_x);
    mkb::GXSetTevColor(mkb::GX_TEVREG0, req->mul_color);
    mkb::GXSetTevColor(mkb::GX_TEVREG1, req->add_color);

    // Send vertex data
    auto write_vertex = [](Vec* pos, Vec2d* uv) {
        mkb::GXPosition3f32(pos->x, pos->y, pos->z);
        mkb::GXTexCoord2f32(uv->x, uv->y);
    };
    mkb::GXBegin(mkb::GX_QUADS, mkb::GX_VTXFMT7, 4);
    write_vertex(&pos_top_left, &uv_top_left);
    write_vertex(&pos_top_right, &uv_top_right);
    write_vertex(&pos_bottom_right, &uv_bottom_right);
    write_vertex(&pos_bottom_left, &uv_bottom_left);

    mkb::reset_ui_widescreen_scale_mtx();
}

void init() {
    patch::write_branch(relutil::relocate_addr(0x802aeca4),
                        reinterpret_cast<void*>(main::full_debug_text_color));
}

}  // namespace draw
