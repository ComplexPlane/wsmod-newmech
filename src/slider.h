#pragma once

#include "mkb/mkb.h"

namespace slider {

f32 get(const char* name, f32 initial_value, f32 increment = 1.f);
void tick();
void disp();

}  // namespace slider
