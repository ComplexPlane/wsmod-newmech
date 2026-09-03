#pragma once

#include "mkb/mkb.h"
#include "relpatches.h"

namespace gameconf {

constexpr u32 WORLD_COUNT = 10;
constexpr u32 WORLD_STAGE_COUNT = 10;

struct StoryStage {
    u16 stage_id;
    u16 time_limit_frames;
    u8 difficulty;
};

struct Config {
    StoryStage story_stages[WORLD_COUNT][WORLD_STAGE_COUNT];
    // Relative to Config allocation
    u16 stage_name_offsets[relpatches::STAGE_COUNT];
};

extern Config* conf;

}  // namespace gameconf
