#pragma once

namespace custompack {

void init_main_loop();
void init_main_game();
void tick();
void draw_stage();
void draw_view_stage();
void stobj_init();
void stobj_tick();
void on_after_load_stagedef();
void preanim_tick();

}  // namespace custompack
