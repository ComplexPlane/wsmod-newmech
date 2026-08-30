#pragma once

#include "mkb/mkb.h"

namespace custompack::models {

void load_custompack_common_gma();
mkb::GmaModel* find(const char* name);

}  // namespace custompack::models
