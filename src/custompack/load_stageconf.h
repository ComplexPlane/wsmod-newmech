#pragma once

namespace load_stageconf {

// Load stageconf. Called after stagedef loads but before GMA/TPL are loaded so we have game heap
// space for parsing
void on_after_load_stagedef();

}  // namespace load_stageconf
