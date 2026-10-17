/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#include "PM_Private.h"

bool pm_anim_type_valid(pm_anim_type_t type, bool allow_global)
{
    return type >= (allow_global ? PM_ANIM_GLOBAL : PM_ANIM_OVER_LEFT) && type <= PM_ANIM_NONE;
}

bool pm_anim_is_over(pm_anim_type_t type)
{
    return type >= PM_ANIM_OVER_LEFT && type <= PM_ANIM_OVER_BOTTOM;
}

int32_t pm_anim_extent(pm_manager_t * manager, pm_anim_type_t type)
{
    int32_t extent;
    bool vertical = type == PM_ANIM_OVER_TOP || type == PM_ANIM_OVER_BOTTOM ||
                    type == PM_ANIM_MOVE_TOP || type == PM_ANIM_MOVE_BOTTOM;
    lv_obj_update_layout(manager->root);
    extent = vertical ? lv_obj_get_content_height(manager->root) : lv_obj_get_content_width(manager->root);
    if(type == PM_ANIM_OVER_RIGHT || type == PM_ANIM_OVER_BOTTOM ||
       type == PM_ANIM_MOVE_RIGHT || type == PM_ANIM_MOVE_BOTTOM) extent = -extent;
    return extent;
}

void pm_anim_configure(pm_manager_t * manager, bool entering, bool from_drag)
{
    pm_anim_type_t type = manager->transition_animation.type;
    pm_anim_values_t * values = &manager->values;
    bool move = type >= PM_ANIM_MOVE_LEFT && type <= PM_ANIM_MOVE_BOTTOM;
    int32_t extent = pm_anim_extent(manager, type);
    memset(values, 0, sizeof(*values));
    values->vertical = type == PM_ANIM_OVER_TOP || type == PM_ANIM_OVER_BOTTOM ||
                       type == PM_ANIM_MOVE_TOP || type == PM_ANIM_MOVE_BOTTOM;
    values->fade = type == PM_ANIM_FADE_ON;
    lv_obj_set_pos(manager->current->root, 0, 0);
    lv_obj_set_style_opa(manager->current->root, LV_OPA_COVER, LV_PART_MAIN);
    if(manager->outgoing) {
        lv_obj_set_pos(manager->outgoing->root, 0, 0);
        lv_obj_set_style_opa(manager->outgoing->root, LV_OPA_COVER, LV_PART_MAIN);
    }
    if(values->fade) {
        values->incoming_start = entering ? LV_OPA_TRANSP : LV_OPA_COVER;
        values->incoming_end = LV_OPA_COVER;
        values->outgoing_start = LV_OPA_COVER;
        values->outgoing_end = entering ? LV_OPA_COVER : LV_OPA_TRANSP;
    }
    else if(type != PM_ANIM_NONE) {
        values->incoming_start = entering ? extent : (move ? -extent : 0);
        values->incoming_end = 0;
        values->outgoing_start = from_drag ? manager->drag_offset : 0;
        values->outgoing_end = entering ? (move ? -extent : 0) : extent;
    }
    if(from_drag) {
        int64_t total = extent < 0 ? -(int64_t)extent : extent;
        int64_t offset = manager->drag_offset < 0 ? -(int64_t)manager->drag_offset : manager->drag_offset;
        if(total > 0) {
            manager->transition_animation.duration_ms = (uint32_t)
                ((int64_t)manager->transition_animation.duration_ms * (total - offset) / total);
        }
        manager->dragging = false;
        manager->drag_below = NULL;
        manager->drag_indev = NULL;
    }
}

static int32_t interpolate(int32_t start, int32_t end, int32_t progress)
{
    return start + (int32_t)(((int64_t)end - start) * progress / PM_ANIM_PROGRESS);
}

static void apply_value(lv_obj_t * root, const pm_anim_values_t * values, int32_t value)
{
    if(values->fade) {
        if(value < LV_OPA_TRANSP) value = LV_OPA_TRANSP;
        if(value > LV_OPA_COVER) value = LV_OPA_COVER;
        /* Object opacity includes the entire subtree, unlike bg_opa. */
        lv_obj_set_style_opa(root, (lv_opa_t)value, LV_PART_MAIN);
    }
    else if(values->vertical) lv_obj_set_y(root, value);
    else lv_obj_set_x(root, value);
}

static void animation_exec(void * data, int32_t progress)
{
    pm_manager_t * manager = data;
    pm_anim_values_t * values = &manager->values;
    bool was_in_operation = manager->in_operation;
    manager->in_operation = true;
    apply_value(manager->current->root, values,
                interpolate(values->incoming_start, values->incoming_end, progress));
    if(manager->motion == PM_MOTION_SWITCH && manager->outgoing) {
        apply_value(manager->outgoing->root, values,
                    interpolate(values->outgoing_start, values->outgoing_end, progress));
    }
    manager->in_operation = was_in_operation;
}

static void animation_finished(lv_anim_t * animation)
{
    pm_manager_t * manager = lv_anim_get_user_data(animation);
    if(manager->motion == PM_MOTION_NONE) return;
    animation_exec(manager, PM_ANIM_PROGRESS);
    if(manager->motion == PM_MOTION_SWITCH) pm_finish_switch(manager);
    else pm_drag_finish(manager);
}

static void start_animation(pm_manager_t * manager)
{
    lv_anim_t animation;
    if(manager->transition_animation.type == PM_ANIM_NONE ||
       manager->transition_animation.duration_ms == 0) {
        animation_exec(manager, PM_ANIM_PROGRESS);
        if(manager->motion == PM_MOTION_SWITCH) pm_finish_switch(manager);
        else pm_drag_finish(manager);
        return;
    }
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, manager);
    lv_anim_set_user_data(&animation, manager);
    lv_anim_set_values(&animation, 0, PM_ANIM_PROGRESS);
    lv_anim_set_duration(&animation, manager->transition_animation.duration_ms);
    lv_anim_set_path_cb(&animation, manager->transition_animation.path);
    lv_anim_set_exec_cb(&animation, animation_exec);
    /* deleted_cb also settles a transition explicitly cancelled by LVGL.
     * At natural completion completed_cb runs first and clears motion. */
    lv_anim_set_completed_cb(&animation, animation_finished);
    lv_anim_set_deleted_cb(&animation, animation_finished);
    if(!lv_anim_start(&animation)) {
        /* If animation allocation fails, complete navigation synchronously. */
        animation_exec(manager, PM_ANIM_PROGRESS);
        if(manager->motion == PM_MOTION_SWITCH) pm_finish_switch(manager);
        else pm_drag_finish(manager);
    }
}

void pm_anim_start_switch(pm_manager_t * manager)
{
    manager->motion = PM_MOTION_SWITCH;
    animation_exec(manager, 0);
    start_animation(manager);
}

void pm_anim_stop(pm_manager_t * manager)
{
    /* Suppress deleted_cb while clear/delete explicitly settle page states. */
    manager->motion = PM_MOTION_NONE;
    lv_anim_delete(manager, animation_exec);
}

void pm_anim_reset_drag(pm_manager_t * manager)
{
    int64_t extent;
    int64_t offset;
    if(manager->drag_offset == 0) {
        pm_drag_finish(manager);
        return;
    }
    manager->dragging = false;
    manager->motion = PM_MOTION_DRAG_RESET;
    manager->transition_animation = manager->current->entry_animation;
    memset(&manager->values, 0, sizeof(manager->values));
    manager->values.vertical = manager->transition_animation.type == PM_ANIM_OVER_TOP ||
                               manager->transition_animation.type == PM_ANIM_OVER_BOTTOM;
    manager->values.incoming_start = manager->drag_offset;
    extent = manager->drag_extent < 0 ? -(int64_t)manager->drag_extent : manager->drag_extent;
    offset = manager->drag_offset < 0 ? -(int64_t)manager->drag_offset : manager->drag_offset;
    if(extent > 0) {
        manager->transition_animation.duration_ms = (uint32_t)
            ((int64_t)manager->transition_animation.duration_ms * offset / extent);
    }
    start_animation(manager);
}
