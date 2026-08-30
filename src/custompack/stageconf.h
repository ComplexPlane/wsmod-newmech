#pragma once

#include "mkb/mkb.h"
#include "vector.h"

namespace custompack::stageconf {

enum class ThingType {
    Foo,
    Bar,
    Bas,
};

struct Thing {
    Vec pos;
    S16Vec rot;
    bool flag;
    ThingType type;
};

struct ItemGroup {
    Vector<Thing> things;
};

struct StageConf {
    ItemGroup* itemgroups;
    u32 itemgroup_count;
};

// Stageconf loaded after stagedef loads, freed when unloading stage
extern StageConf* conf;

}  // namespace custompack::stageconf
