#include "assembly.h"
#include "mkb/mkb.h"

namespace main {

u16 bgm_id_lookup[relpatches::STAGE_COUNT] = {0};
u16 theme_id_lookup[relpatches::STAGE_COUNT] = {0};
mkb::GXColor debug_text_color;

}  // namespace main
