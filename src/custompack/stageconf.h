#pragma once

#include "containers.h"
#include "mkb/mkb.h"

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
    cnt::Vector<Thing> things;
};

struct StageConf {
    cnt::Vector<ItemGroup> itemgroups;
};

// Stageconf loaded after stagedef loads, freed when unloading stage
extern StageConf* conf;

}  // namespace custompack::stageconf
