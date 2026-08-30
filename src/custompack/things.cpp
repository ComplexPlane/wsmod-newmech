#include "things.h"

#include "custompack/model_utils.h"
#include "custompack/stageconf.h"
#include "math_utils.h"
#include "mem.h"
#include "mkb/mkb.h"
#include "slider.h"

using namespace mathutils;

namespace {

constexpr f32 HITBOX_RADIUS = 0.6f;

enum ThingFlags {
    ThingFlag_Collided = 1 << 0,
};

struct ThingInst {
    custompack::stageconf::Thing* conf;
    u8 flags;
    u16 itemgroup_idx;
};

struct ShadowReq {
    Vec pos;
    S16Vec rot;
    Vec scale;
    mkb::GmaModel* model;
    f32 alpha;
};

ThingInst* s_things = nullptr;
u32 s_thing_count = 0;
mkb::GmaModel* s_thing_model = nullptr;

Vec pos_world_from_ig(Vec pos, u32 itemgroup_idx) {
    mkb::mtxa_push();
    if (itemgroup_idx > 0) {
        mkb::mtxa_from_mtx(&mkb::itemgroups[itemgroup_idx].transform);
    } else {
        mkb::mtxa_from_identity();
    }
    Vec pos_rt_world = {};
    mkb::mtxa_tf_point(&pos, &pos_rt_world);
    mkb::mtxa_pop();
    return pos_rt_world;
}

// Requires mtxb to be set to view-from-world matrix (only set during draw funcs)
f32 get_camera_distance(Vec pos_rt_world) {
    mkb::mtxa_push();
    mkb::mtxa_from_mtxb();
    Vec pos_rt_view = {};
    mkb::mtxa_tf_point(&pos_rt_world, &pos_rt_view);
    mkb::mtxa_pop();
    return -pos_rt_view.z;
}

void draw_shadow_req(ShadowReq* shadow) {
    mkb::avdisp_set_z_mode(mkb::GX_TRUE, mkb::GX_LEQUAL, mkb::GX_FALSE);

    mkb::mtxa_from_mtxb_translate(&shadow->pos);
    mkb::mtxa_rotate_y((shadow->rot).y);
    mkb::mtxa_rotate_x((shadow->rot).x);
    mkb::mtxa_rotate_z((shadow->rot).z);
    mkb::mtxa_scale(&shadow->scale);
    mkb::set_post_mult_color(shadow->alpha, shadow->alpha, shadow->alpha, 1.f);

    mkb::load_gx_pos_nrm_mtx(mkb::mtxa, 0);
    mkb::avdisp_draw_model_culled_sort_never(shadow->model);
    mkb::set_post_mult_color(1.f, 1.f, 1.f, 1.f);

    mkb::avdisp_set_z_mode(mkb::GX_TRUE, mkb::GX_LEQUAL, mkb::GX_TRUE);
}

void draw_thing_shadow(ThingInst* thing, f32 thing_alpha) {
    f32 scale = slider::get("shadow scale", 0.55, 0.05);
    f32 base_alpha = slider::get("shadow alpha", 0.40, 0.05);
    f32 min_shadow_dist = slider::get("min sdw dist", 50);
    f32 max_shadow_dist = slider::get("max sdw dist", 60);

    Vec pos_rt_world = pos_world_from_ig(thing->conf->pos, thing->itemgroup_idx);
    f32 dist = get_camera_distance(pos_rt_world);
    if (dist > 0 && dist < max_shadow_dist) {
        mkb::RaycastHit raycast = {};
        Vec dummy = {};
        mkb::raycast_stage_down(&pos_rt_world, &raycast, &dummy);

        if (raycast.flags & 1) {
            ShadowReq shadow = {};
            shadow.model = mkb::init_common_gma->model_entries[0x67].model;
            shadow.pos = raycast.pos;
            shadow.pos.y += 0.01;  // I couldn't work out how SMB's shadow system avoids Z fighting.
            mkb::vec_to_euler(&raycast.normal, &shadow.rot);
            shadow.scale = {scale, scale, scale};
            f32 dist_t = clamp(inv_lerp(dist, max_shadow_dist, min_shadow_dist), 0.f, 1.f);
            shadow.alpha = base_alpha * thing_alpha * dist_t;
            draw_shadow_req(&shadow);
        }
    }
}

void draw_base(ThingInst* thing, s16 rot, f32 alpha) {
    mkb::mtxa_from_mtxb();
    if (thing->itemgroup_idx > 0) {
        mkb::mtxa_mult_right(&mkb::itemgroups[thing->itemgroup_idx].transform);
    }
    mkb::mtxa_translate(&thing->conf->pos);
    mkb::mtxa_rotate_z(thing->conf->rot.z);
    mkb::mtxa_rotate_y(thing->conf->rot.y);
    mkb::mtxa_rotate_x(thing->conf->rot.x);
    mkb::mtxa_rotate_y(rot);
    mkb::load_gx_pos_nrm_mtx(mkb::mtxa, 0);

    mkb::avdisp_set_alpha(alpha);
    mkb::avdisp_draw_model_culled_sort_auto(s_thing_model);
}

void draw_thing(ThingInst* thing, s16 rot) {
    constexpr f32 ALPHA = 1.f;
    draw_base(thing, rot, ALPHA);
    draw_thing_shadow(thing, ALPHA);
}

void collide_things() {
    for (u32 i = 0; i < s_thing_count; i++) {
        // Collision pass: iterate over all things checking for collision, and mark all
        // things that _should_ activate. This is O(n^2) if many things are contacted at
        // the same time but otherwise OK

        ThingInst* thing = &s_things[i];

        Vec thing_pos = thing->conf->pos;
        if (thing->itemgroup_idx > 0) {
            mkb::mtxa_from_mtx(&mkb::itemgroups[thing->itemgroup_idx].transform);
            mkb::mtxa_tf_point(&thing_pos, &thing_pos);
        }

        mkb::Ball* ball = &mkb::balls[mkb::curr_player_idx];
        f32 dist_sq = vec_dist_sq(ball->pos, thing_pos);
        f32 radii_sum = ball->physical_ball_radius + HITBOX_RADIUS;

        if (dist_sq < radii_sum * radii_sum) {
            // Respond to collision
            thing->flags |= ThingFlag_Collided;
        }
    }
}

}  // namespace

namespace custompack::things {

void on_after_load_stagedef() {
    // This really only needs to run after common gma/tpl are loaded aka heaps reset
    s_thing_model = custompack::models::find("THING");
}

void tick() {
}

void stobj_init() {
    // Preallocate Things array
    s_thing_count = 0;
    for (u32 ig_idx = 0; ig_idx < stageconf::conf->itemgroup_count; ig_idx++) {
        stageconf::ItemGroup* ig_conf = &stageconf::conf->itemgroups[ig_idx];
        s_thing_count += ig_conf->thing_count;
    }
    s_things = mem::gameplay_arena.alloc_array<ThingInst>(s_thing_count);

    // Initialize Things array
    u32 thing_idx = 0;
    for (u32 ig_idx = 0; ig_idx < stageconf::conf->itemgroup_count; ig_idx++) {
        stageconf::ItemGroup* ig_conf = &stageconf::conf->itemgroups[ig_idx];

        for (u32 i = 0; i < ig_conf->thing_count; i++) {
            stageconf::Thing* thing_conf = &ig_conf->things[i];
            s_things[thing_idx].conf = thing_conf;
            s_things[thing_idx].itemgroup_idx = ig_idx;

            thing_idx++;
        }
    }
}

void stobj_tick() {
    collide_things();
}

void draw_stage() {
    for (u32 i = 0; i < s_thing_count; i++) {
        draw_thing(&s_things[i], 0);
    }
}

void draw_view_stage() {
    for (u32 i = 0; i < s_thing_count; i++) {
        ThingInst thing = s_things[i];
        thing.flags = 0;
        draw_thing(&thing, 0);
    }
}

}  // namespace custompack::things
