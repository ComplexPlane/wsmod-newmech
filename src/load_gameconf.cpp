#include "load_gameconf.h"
#include "assembly.h"
#include "gameconf.h"
#include "json_utils.h"
#include "relpatches.h"

#include "mkb/mkb.h"

namespace gameconf {

namespace {

[[noreturn]] void unknown_key(sj_Reader* reader, sj_Value key) {
    int line = 1;
    for (char* p = reader->data; p != key.start; p++) {
        if (*p == '\n') {
            line++;
        }
    }

    s32 length = key.end - key.start;
    ABORT_MSG("config.json @ line %d: unknown key: '%.*s'", line, length, key.start);
}

char* push_str(arena::Arena* arena, gameconf::Config* conf, sj_Value str) {
    ASSERT(str.type == SJ_STRING);
    u32 size = str.end - str.start;
    char* buf = (char*)arena->alloc_bytes(size + 1, 1);
    mkb::memcpy(buf, str.start, size);
    return buf;
}

void parse_story_stage(arena::Arena* arena, sj_Reader* reader, sj_Value stage, u32 world_idx,
                       u32 stage_idx, gameconf::Config* conf) {
    ASSERT(world_idx < gameconf::WORLD_COUNT);
    ASSERT(stage_idx < gameconf::WORLD_STAGE_COUNT);

    gameconf::StoryStage* story_stage = &conf->story_stages[world_idx][stage_idx];

    u16 theme_id = 0;
    u16 music_id = 0;
    char* name = nullptr;

    sj_Value key = {};
    sj_Value value = {};
    while (sj_iter_object(reader, stage, &key, &value)) {
        if (json::eq(key, "stage_id")) {
            story_stage->stage_id = json::parse_int(value);
        } else if (json::eq(key, "name")) {
            name = push_str(arena, conf, value);
        } else if (json::eq(key, "theme_id")) {
            theme_id = json::parse_int(value);
        } else if (json::eq(key, "music_id")) {
            music_id = json::parse_int(value);
        } else if (json::eq(key, "time_limit")) {
            // Close enough approximation of round()
            story_stage->time_limit_frames = json::parse_float(value) * 60.f + 0.5f;
        } else if (json::eq(key, "difficulty")) {
            story_stage->difficulty = json::parse_int(value);
        } else {
            unknown_key(reader, key);
        }
    }

    ASSERT(story_stage->stage_id < relpatches::STAGE_COUNT);
    main::theme_id_lookup[story_stage->stage_id] = theme_id;
    main::bgm_id_lookup[story_stage->stage_id] = music_id;

    if (name != nullptr) {
        u32 offset = (u32)name - (u32)conf;
        conf->stage_name_offsets[story_stage->stage_id] = offset;
    }
}

void parse_party_game_toggles(sj_Reader* reader, sj_Value toggles) {
    sj_Value key = {};
    sj_Value value = {};
    while (sj_iter_object(reader, toggles, &key, &value)) {
        bool yes_disable = json::parse_bool(value);
        u32 flag = 0;
        if (json::eq(key, "race")) {
            flag |= 0x1;
        } else if (json::eq(key, "fight")) {
            flag |= 0x2;
        } else if (json::eq(key, "target")) {
            flag |= 0x4;
        } else if (json::eq(key, "billiards")) {
            flag |= 0x8;
        } else if (json::eq(key, "bowling")) {
            flag |= 0x10;
        } else if (json::eq(key, "golf")) {
            flag |= 0x20;
        } else if (json::eq(key, "boat")) {
            flag |= 0x40;
        } else if (json::eq(key, "shot")) {
            flag |= 0x80;
        } else if (json::eq(key, "dogfight")) {
            flag |= 0x100;
        } else if (json::eq(key, "soccer")) {
            flag |= 0x200;
        } else if (json::eq(key, "baseball")) {
            flag |= 0x400;
        } else if (json::eq(key, "tennis")) {
            flag |= 0x800;
        } else {
            unknown_key(reader, key);
        }

        if (yes_disable) {
            relpatches::party_game_toggle::party_game_bitflag |= flag;
        }
    }
}

void parse_patches(sj_Reader* reader, sj_Value patches) {
    sj_Value key = {};
    sj_Value value = {};
    while (sj_iter_object(reader, patches, &key, &value)) {
        // Find matching patch
        relpatches::Tickable* patch = nullptr;
        for (u32 i = 0; i < relpatches::PATCH_COUNT; i++) {
            relpatches::Tickable* curr_patch = &relpatches::patches[i];
            if (curr_patch->name != nullptr && json::eq(key, curr_patch->name)) {
                patch = curr_patch;
            }
        }
        if (patch == nullptr) {
            unknown_key(reader, key);
        }

        // Parse patch value
        if (value.type == SJ_BOOL) {
            patch->status = json::parse_bool(value);

        } else if (value.type == SJ_NUMBER) {
            s32 parsed_value = json::parse_int(value);
            if (parsed_value < patch->minimum_value) {
                ABORT_MSG("Passed value for patch '%s' is smaller than minimum value", patch->name);
            }
            if (parsed_value > patch->maximum_value) {
                ABORT_MSG("Passed value for patch '%s' larger than maximum value", patch->name);
            }
            patch->status = parsed_value;

        } else {
            ABORT_MSG("Invalid patch value for patch '%s'", patch->name);
        }
    }
}

}  // namespace

void load(arena::Arena* arena) {
    // Game heaps don't exist yet, so do temp allocations on mkb arena
    void* orig_arena_lo = mkb::OSGetArenaLo();
    u32 config_json_size = 0;
    auto allocator = [](u32 size) {
        u32 rounded_up_size = mkb::OSRoundUp32B(size);
        return mkb::OSAllocFromArenaLo(rounded_up_size, 32);
    };
    void* config_json = json::read_file(allocator, "config.json", &config_json_size);
    if (config_json == nullptr) {
        ABORT_MSG("Failed to load config.json");
    }

    gameconf::Config* parsed_conf = arena->alloc_struct<gameconf::Config>();
    sj_Reader reader = sj_reader((char*)config_json, config_json_size);
    sj_Value root = sj_read(&reader);
    sj_Value root_child_key = {};
    sj_Value root_child_value = {};
    while (sj_iter_object(&reader, root, &root_child_key, &root_child_value)) {
        if (json::eq(root_child_key, "story_mode_stages")) {
            u32 world_idx = 0;
            sj_Value world = {};
            while (sj_iter_array(&reader, root_child_value, &world)) {
                u32 stage_idx = 0;
                sj_Value stage = {};
                while (sj_iter_array(&reader, world, &stage)) {
                    parse_story_stage(arena, &reader, stage, world_idx, stage_idx, parsed_conf);
                    stage_idx++;
                }
                world_idx++;
            }

        } else if (json::eq(root_child_key, "party_game_toggles")) {
            parse_party_game_toggles(&reader, root_child_value);

        } else if (json::eq(root_child_key, "patches")) {
            parse_patches(&reader, root_child_value);

        } else {
            unknown_key(&reader, root_child_key);
        }
    }

    if (reader.error != nullptr) {
        int line = 0;
        int col = 0;
        sj_location(&reader, &line, &col);
        ABORT_MSG("config.json JSON error: line %d col %d: %s", line, col, reader.error);
    }

    mkb::OSSetArenaLo(orig_arena_lo);

    gameconf::conf = parsed_conf;
}

}  // namespace gameconf
