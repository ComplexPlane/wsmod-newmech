#include "slider.h"
#include "draw.h"
#include "logging.h"
#include "math_utils.h"
#include "mkb/mkb2_ghidra.h"
#include "pad.h"

namespace slider {

namespace {

constexpr bool SHOW_SLIDERS = false;

struct Slider {
    const char* name;
    f32 value;
    f32 inc;
};

constexpr s32 MAX_SLIDERS = 16;

Slider s_sliders[MAX_SLIDERS] = {};
s32 s_slider_count = 0;
s32 s_selected_slider = 0;

}  // namespace

f32 get(const char* name, f32 initial_value, f32 increment) {
    Slider* slider = nullptr;
    for (s32 i = 0; i < s_slider_count; i++) {
        if (mkb::strcmp((char*)s_sliders[i].name, (char*)name) == 0) {
            slider = &s_sliders[i];
        }
    }
    if (slider == nullptr) {
        ASSERT(s_slider_count < (s32)LEN(s_sliders));
        slider = &s_sliders[s_slider_count++];
        slider->name = name;
        slider->value = initial_value;
        slider->inc = increment;
    }

    return slider->value;
}

void tick() {
    if (SHOW_SLIDERS) {
        if (s_slider_count == 0) {
            return;
        }
        if (pad::button_pressed(mkb::PAD_BUTTON_UP)) {
            s_selected_slider =
                mathutils::clamp(s_selected_slider - 1, (s32)0, (s32)(s_slider_count - 1));
        }
        if (pad::button_pressed(mkb::PAD_BUTTON_DOWN)) {
            s_selected_slider =
                mathutils::clamp(s_selected_slider + 1, (s32)0, (s32)(s_slider_count - 1));
        }
        if (pad::button_repeated(mkb::PAD_BUTTON_LEFT)) {
            Slider* slider = &s_sliders[s_selected_slider];
            slider->value -= slider->inc;
        }
        if (pad::button_repeated(mkb::PAD_BUTTON_RIGHT)) {
            Slider* slider = &s_sliders[s_selected_slider];
            slider->value += slider->inc;
        }
    }
}

void disp() {
    if (SHOW_SLIDERS) {
        for (s32 i = 0; i < s_slider_count; i++) {
            Slider* slider = &s_sliders[i];
            const char* prefix = i == s_selected_slider ? "->" : "  ";
            draw::debug_text(400, 10 + i * 15, draw::GREEN, "%s %s = %.2f", prefix, slider->name,
                             slider->value);
        }
    }
}
}  // namespace slider
