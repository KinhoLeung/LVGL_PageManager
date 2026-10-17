/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#include "PM_Private.h"

void pm_drag_finish(pm_manager_t * manager)
{
    bool was_in_operation = manager->in_operation;
    manager->in_operation = true;
    if(manager->drag_below && manager->drag_below->root) {
        lv_obj_add_flag(manager->drag_below->root, LV_OBJ_FLAG_HIDDEN);
    }
    if(manager->current && manager->current->root) lv_obj_set_pos(manager->current->root, 0, 0);
    manager->dragging = false;
    manager->drag_moved = false;
    manager->drag_below = NULL;
    manager->drag_indev = NULL;
    manager->drag_offset = 0;
    manager->drag_velocity = 0;
    manager->motion = PM_MOTION_NONE;
    manager->busy = false;
    manager->in_operation = was_in_operation;
}

static void drag_begin(pm_page_t * page, lv_indev_t * indev)
{
    pm_manager_t * manager = page->manager;
    pm_page_t * below = page->stack_previous;
    if(manager->busy || manager->in_operation || !manager->drag_enabled ||
       manager->current != page || !below || !below->root ||
       !pm_anim_is_over(page->entry_animation.type) || !indev ||
       lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) return;
    manager->in_operation = true;
    manager->drag_extent = pm_anim_extent(manager, page->entry_animation.type);
    if(manager->drag_extent == 0) {
        manager->in_operation = false;
        return;
    }
    manager->dragging = true;
    manager->drag_moved = false;
    manager->busy = true;
    manager->drag_offset = 0;
    manager->drag_velocity = 0;
    manager->drag_below = below;
    manager->drag_indev = indev;
    lv_obj_set_pos(below->root, 0, 0);
    lv_obj_set_style_opa(below->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(below->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_to_index(below->root, -1);
    lv_obj_move_to_index(page->root, -1);
    manager->in_operation = false;
}

static void drag_move(pm_manager_t * manager)
{
    lv_point_t vector;
    int64_t offset;
    bool vertical = manager->current->entry_animation.type == PM_ANIM_OVER_TOP ||
                    manager->current->entry_animation.type == PM_ANIM_OVER_BOTTOM;
    lv_indev_get_vect(manager->drag_indev, &vector);
    manager->drag_velocity = vertical ? vector.y : vector.x;
    offset = (int64_t)manager->drag_offset + manager->drag_velocity;
    if(manager->drag_extent > 0) {
        if(offset < 0) offset = 0;
        if(offset > manager->drag_extent) offset = manager->drag_extent;
    }
    else {
        if(offset > 0) offset = 0;
        if(offset < manager->drag_extent) offset = manager->drag_extent;
    }
    manager->drag_offset = (int32_t)offset;
    if(offset) manager->drag_moved = true;
    manager->in_operation = true;
    if(vertical) lv_obj_set_y(manager->current->root, manager->drag_offset);
    else lv_obj_set_x(manager->current->root, manager->drag_offset);
    manager->in_operation = false;
}

static void drag_release(pm_manager_t * manager, bool cancelled)
{
    int64_t prediction = manager->drag_offset;
    int32_t velocity = manager->drag_velocity;
    int64_t extent = manager->drag_extent;
    while(velocity) {
        prediction += velocity;
        velocity = (int32_t)((int64_t)velocity * 80 / 100);
    }
    if(extent < 0) {
        extent = -extent;
        prediction = -prediction;
    }
    if(!cancelled && manager->drag_moved && prediction * 100 >= extent * manager->drag_threshold) {
        /* No code may use page/root after this call: a zero-duration pop can
         * synchronously delete the root that dispatched LV_EVENT_RELEASED. */
        if(pm_pop_internal(manager, true)) return;
    }
    pm_anim_reset_drag(manager);
}

void pm_drag_event(lv_event_t * event)
{
    pm_page_t * page = lv_event_get_user_data(event);
    pm_manager_t * manager;
    lv_event_code_t code = lv_event_get_code(event);
    if(code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
       code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) return;
    /* Child controls keep their own gestures. Drag on the root background. */
    if(lv_event_get_target(event) != lv_event_get_current_target(event)) return;
    manager = page->manager;
    if(code == LV_EVENT_PRESSED) {
        drag_begin(page, lv_event_get_indev(event));
        return;
    }
    if(!manager->dragging || manager->current != page || manager->in_operation) return;
    if(lv_event_get_indev(event) != manager->drag_indev) return;
    if(code == LV_EVENT_PRESSING) drag_move(manager);
    else drag_release(manager, code == LV_EVENT_PRESS_LOST);
}
