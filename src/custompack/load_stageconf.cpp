#include "load_stageconf.h"

#include "math_utils.h"
#include "mem.h"

#include "custompack/stageconf.h"
#include "json_utils.h"
#include "logging.h"
#include "mkb/mkb.h"

namespace load_stageconf {

namespace {

const char* THING_TYPES[] = {
    "FOO",
    "BAR",
    "BAS",
};

void parse_things(sj_Reader* reader, u32 itemgroup_idx, sj_Value things_json) {
    custompack::stageconf::ItemGroup* itemgroup = &custompack::stageconf::conf->itemgroups[itemgroup_idx];
    itemgroup->thing_count = json::parse_array_len(*reader, things_json);
    itemgroup->things = mem::stage_arena.alloc_array<custompack::stageconf::Thing>(itemgroup->thing_count);

    u32 thing_idx = 0;
    sj_Value thing_json = {};
    while (sj_iter_array(reader, things_json, &thing_json)) {
        custompack::stageconf::Thing* thing = &itemgroup->things[thing_idx];

        sj_Value key = {}, value = {};
        while (sj_iter_object(reader, thing_json, &key, &value)) {
            if (json::eq(key, "pos")) {
                thing->pos = json::parse_vec(reader, value);
            } else if (json::eq(key, "rot")) {
                thing->rot = json::parse_rot(reader, value);
            } else if (json::eq(key, "type")) {
                thing->type = (custompack::stageconf::ThingType)json::parse_enum(value, THING_TYPES,
                                                                           LEN(THING_TYPES));
            }
        }

        thing_idx++;
    }
}

void parse_itemgroup(sj_Reader* reader, u32 itemgroup_idx, sj_Value itemgroup_json) {
    sj_Value key = {}, value = {};
    while (sj_iter_object(reader, itemgroup_json, &key, &value)) {
        if (json::eq(key, "things")) {
            parse_things(reader, itemgroup_idx, value);
        }
    }
}

// Returns the `itemgroups` top-level field's array value if present.
// Otherwise, returns an error value.
sj_Value parse_itemgroups_field(sj_Reader* reader) {
    sj_Value root_json = sj_read(reader);
    if (root_json.type == SJ_ERROR) {
        return sj_Value{};  // Error value
    }

    sj_Value itemgroups_json = {};
    sj_Value key = {}, value = {};
    while (sj_iter_object(reader, root_json, &key, &value)) {
        if (json::eq(key, "itemgroups")) {
            itemgroups_json = value;
            break;
        }
    }
    if (itemgroups_json.type == SJ_ERROR) {
        return sj_Value{};  // Error value
    }

    return itemgroups_json;
}

void load_stageconf() {
    auto conf = mem::stage_arena.alloc_struct<custompack::stageconf::StageConf>();
    custompack::stageconf::conf = conf;
    conf->itemgroups =
        mem::stage_arena.alloc_array<custompack::stageconf::ItemGroup>(mkb::stagedef->coli_header_count);
    conf->itemgroup_count = mkb::stagedef->coli_header_count;

    char stageconf_path[32] = {};
    // DVD current dir is "stage"
    mkb::sprintf(stageconf_path, "st%03d.json", mkb::g_stage_id_to_load);
    u32 json_text_size = 0;
    char* json_text = static_cast<char*>(
        json::read_file([](u32 size) { return mkb::OSAllocFromHeap(mkb::stage_heap, size); },
                        stageconf_path, &json_text_size));

    if (json_text == nullptr) {
        // No JSON file present, parse fake empty JSON object.
        for (u32 i = 0; i < mkb::stagedef->coli_header_count; i++) {
            sj_Reader dummy_reader = sj_reader("{}", 2);
            sj_Value dummy_json = sj_read(&dummy_reader);
            parse_itemgroup(&dummy_reader, i, dummy_json);
        }

    } else {
        // Parse real itemgroups array
        sj_Reader reader = sj_reader(json_text, json_text_size);
        sj_Value itemgroups_json = parse_itemgroups_field(&reader);

        u32 conf_itemgroup_count = json::parse_array_len(reader, itemgroups_json);
        ASSERT(conf_itemgroup_count == mkb::stagedef->coli_header_count);

        u32 i = 0;
        sj_Value itemgroup_json;
        while (sj_iter_array(&reader, itemgroups_json, &itemgroup_json)) {
            parse_itemgroup(&reader, i, itemgroup_json);
            i++;
        }

        mkb::OSFreeToHeap(mkb::stage_heap, json_text);
    }
}

}  // namespace

void on_after_load_stagedef() {
    load_stageconf();
}

}  // namespace load_stageconf
