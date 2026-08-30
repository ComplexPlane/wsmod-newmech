#include "world_timer.h"

#include "relutil.h"

// TODO:
// View Stage
// Replays

namespace world_timer {

namespace {

u32 s_timer = 0;

}  // namespace

void tick() {
    // This is when itemgroup animation frame advances.
    bool paused_now = *reinterpret_cast<u32*>(relutil::relocate_addr(0x805BC474)) &
                      8;  // TODO actually give this a name
    if (mkb::mode_info.ball_mode & mkb::BALLMODE_IN_STAGE_LOADIN &&
        mkb::sub_mode_frame_counter < 361) {
        s_timer = (360 - mkb::sub_mode_frame_counter);
    } else if (!paused_now && !(mkb::g_some_gameplay_flags & 4)) {
        // TODO use some "gameplay timer" possibly in
        // mode_info instead of our own counter?
        s_timer++;
    }
}

u32 get() {
    return s_timer;
}

}  // namespace world_timer
