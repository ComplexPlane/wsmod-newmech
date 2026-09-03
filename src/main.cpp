#include "mkb/mkb.h"

#include "assembly.h"
#include "cardio.h"
#include "load_gameconf.h"
#include "mem.h"
#include "modlink.h"
#include "pad.h"
#include "patch.h"
#include "relpatches.h"
#include "relutil.h"
#include "version.h"

#include "custompack/custompack.h"

namespace main {

namespace {

const version::SemVer CUSTOMPACK_VERSION = {1, 0, 0};

void do_assembly_patches() {
    // Inject the run function at the start of the main game loop
    patch::write_branch_bl(relutil::relocate_addr(0x80270700),
                           reinterpret_cast<void*>(start_main_loop_assembly));

    /* Remove OSReport call ``PERF : event is still open for CPU!``
    since it reports every frame, and thus clutters the console */
    // Only needs to be applied to the US version
    patch::write_nop(relutil::relocate_addr(0x80033E9C));

    // Nop the conditional that guards `draw_debugtext`, enabling it even when debug mode is
    // disabled
    patch::write_nop(relutil::relocate_addr(0x80299f54));
}

typedef void (*TickableFunc)();
typedef TickableFunc (*TickableFuncFunc)(const relpatches::Tickable& tickable);

// Run a specific Tickable function callback when appropriate
void run_tickable_func(TickableFuncFunc tickable_func_func) {
    for (u32 patch_idx = 0; patch_idx < relpatches::PATCH_COUNT; patch_idx++) {
        auto f = tickable_func_func(relpatches::patches[patch_idx]);
        if (relpatches::patches[patch_idx].status && f != nullptr) {
            f();
        }
    }
    for (u32 module_idx = 0; module_idx < relpatches::MODULE_COUNT; module_idx++) {
        auto f = tickable_func_func(relpatches::modules[module_idx]);
        if (f != nullptr) {
            f();
        }
    }
}

void view_stage_draw_hook() {
    run_tickable_func([](auto tickable) { return tickable.draw_view_stage_func; });
    mkb::ord_tbl_draw_nodes();
}

void stobj_tick_hook() {
    mkb::g_stobj_sound_something();
    run_tickable_func([](auto tickable) { return tickable.stobj_tick_func; });
}

void on_did_load_stagedef_hook(u32 stage_id) {
    mkb::load_stagedef(stage_id);

    if (stage_id != 0) {
        mem::stage_arena.reset();
        custompack::on_after_load_stagedef();

        // Allocate gameplay arena from remaining space in stage arena (after stageconf)
        u32 size = 0;
        void* start = mem::stage_arena.alloc_remaining_bytes(32, &size);
        mem::gameplay_arena.init("gameplay", start, size);
    }
}

TRAMP(s_draw_debugtext_tramp, mkb::draw_debugtext, []() {
    // Drawing hook for UI elements.
    // Gets run at the start of smb2's function which draws debug text windows,
    // which is called at the end of smb2's function which draws the UI in general.
    // Disp functions (REL patches)
    run_tickable_func([](auto tickable) { return tickable.disp_func; });
    s_draw_debugtext_tramp.chain();
});

TRAMP(s_process_inputs_tramp, mkb::process_inputs, []() {
    s_process_inputs_tramp.chain();
    pad::tick();
    run_tickable_func([](auto tickable) { return tickable.tick_func; });
});

TRAMP(s_OSLink_tramp, mkb::OSLink, [](mkb::OSModuleHeader* rel_buffer, void* bss_buffer) {
    bool ret = s_OSLink_tramp.chain(rel_buffer, bss_buffer);

    // Main game init functions
    if (rel_buffer->info.id == relutil::ModuleId::MainGame) {
        // Call just before depth-sorted view stage draw calls are drawn
        patch::write_branch_bl(relutil::relocate_addr(0x80913598),
                               reinterpret_cast<void*>(view_stage_draw_hook));

        run_tickable_func([](auto tickable) { return tickable.main_game_init_func; });
    }

    // Sel_ngc init functions
    else if (rel_buffer->info.id == relutil::ModuleId::SelNgc) {
        run_tickable_func([](auto tickable) { return tickable.sel_ngc_init_func; });
    }

    return ret;
});

TRAMP(s_draw_stage_tramp, mkb::g_draw_stage, [] {
    s_draw_stage_tramp.chain();
    run_tickable_func([](auto tickable) { return tickable.draw_stage_func; });
});

// Stobj hooks
TRAMP(s_stobj_init_tramp, mkb::event_stobj_init, [] {
    s_stobj_init_tramp.chain();

    mem::gameplay_arena.reset();

    run_tickable_func([](auto tickable) { return tickable.stobj_init_func; });
});

TRAMP(s_stobj_dest_tramp, mkb::event_stobj_dest, [] {
    s_stobj_dest_tramp.chain();
    run_tickable_func([](auto tickable) { return tickable.stobj_dest_func; });
});

TRAMP(s_preanim_tramp, mkb::g_advance_stage_animation, [] {
    custompack::preanim_tick();
    s_preanim_tramp.chain();
});

void hook_c_patches() {
    HOOK_TRAMP(s_draw_debugtext_tramp);
    HOOK_TRAMP(s_process_inputs_tramp);
    HOOK_TRAMP(s_OSLink_tramp);
    HOOK_TRAMP(s_draw_stage_tramp);
    HOOK_TRAMP(s_stobj_init_tramp);
    HOOK_TRAMP(s_stobj_dest_tramp);
    HOOK_TRAMP(s_preanim_tramp);

    patch::write_branch_bl(relutil::relocate_addr(0x80317d94),
                           reinterpret_cast<void*>(stobj_tick_hook));
    patch::write_branch_bl(reinterpret_cast<void*>(relutil::relocate_addr(0x802c734c)),
                           reinterpret_cast<void*>(on_did_load_stagedef_hook));
}

void savestate(void* context, u32 flags, modlink::SaveRegionFunc region_func) {
    region_func(context, mem::gameplay_arena.get_start(), mem::gameplay_arena.get_occupied(), 0);
}

}  // namespace

void init() {
    mkb::OSReport((char*)"[wsmod] CustomPack v%d.%d.%d loaded\n", CUSTOMPACK_VERSION.major,
                  CUSTOMPACK_VERSION.minor, CUSTOMPACK_VERSION.patch);

    do_assembly_patches();
    hook_c_patches();

    mem::init_arenas();

    // Do all wsmod arena-related loading
    cardio::init(&mem::wsmod_arena);
    gameconf::load(&mem::wsmod_arena);

    // Called only after wsmod arena is populated
    mem::init_chainload_heap();
    modlink::init();
    modlink::set_savestate_func(savestate);

    // Run all main_loop_init() module funcs
    run_tickable_func([](auto tickable) { return tickable.main_loop_init_func; });
}

/*
 * This runs at the very start of the main game loop. Most per-frame code runs after
 * controller inputs have been read and processed however, to ensure the lowest input delay.
 */
void tick() {
    pad::on_frame_start();
    cardio::tick();
}

}  // namespace main
