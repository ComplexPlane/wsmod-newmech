#include "custompack/model_utils.h"

#include "logging.h"
#include "patch.h"

#include "mkb/mkb.h"

namespace {

mkb::GmaBuffer* s_custompack_objects_gma = nullptr;

}  // namespace

namespace custompack::models {

void load_custompack_common_gma() {
    mkb::TplBuffer* tpl = mkb::g_load_tpl("/init/custompack_objects.tpl");
    s_custompack_objects_gma = mkb::g_load_gma("/init/custompack_objects.gma", tpl);
    // Will be freed when game heap is destroyed
}

mkb::GmaModel* find(const char* name) {
    ASSERT(s_custompack_objects_gma != nullptr);

    if (s_custompack_objects_gma == nullptr) return nullptr;
    for (int i = 0; i < s_custompack_objects_gma->model_count; i++) {
        const char* curr_name = s_custompack_objects_gma->model_entries[i].name;
        if (mkb::strcmp(const_cast<char*>(curr_name), const_cast<char*>(name)) == 0) {
            return s_custompack_objects_gma->model_entries[i].model;
        }
    }
    return nullptr;
}

}  // namespace custompack::models
