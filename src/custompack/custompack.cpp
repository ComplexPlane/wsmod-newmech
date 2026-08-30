#include "custompack.h"
#include "custompack/things.h"
#include "load_stageconf.h"

namespace custompack {

void init_main_loop() {
}

void init_main_game() {
}

void tick() {
    custompack::things::tick();
}

void draw_stage() {
    custompack::things::draw_stage();
}

void draw_view_stage() {
    custompack::things::draw_view_stage();
}

void stobj_init() {
    custompack::things::stobj_init();
}

void stobj_tick() {
    custompack::things::stobj_tick();
}

void preanim_tick() {
}

void on_after_load_stagedef() {
    load_stageconf::on_after_load_stagedef();
    custompack::things::on_after_load_stagedef();
}

}  // namespace custompack
