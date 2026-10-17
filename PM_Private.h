/* SPDX-License-Identifier: MIT -- internal implementation, not a public API. */
#ifndef LVGL_PM_PRIVATE_H
#define LVGL_PM_PRIVATE_H

#include "PageManager.h"
#include <string.h>

#define PM_ANIM_PROGRESS 1024

struct pm_page {
    pm_manager_t * manager;
    pm_page_t * pool_next;
    pm_page_t * stack_previous;
    char * name;
    lv_obj_t * root;
    pm_page_callbacks_t callbacks;
    void * user_data;
    uint32_t id;
    pm_page_state_t state;
    pm_cache_mode_t cache_mode;
    pm_anim_config_t animation;
    /* Snapshot of the animation used to enter; pop reverses that animation. */
    pm_anim_config_t entry_animation;
    void * stash_data;
    size_t stash_size;
    bool in_stack;
    void (*factory_destroy)(void *, void *);
    void * factory_user_data;
};

typedef enum {
    PM_MOTION_NONE = 0,
    PM_MOTION_SWITCH,
    PM_MOTION_DRAG_RESET
} pm_motion_t;

typedef struct {
    bool vertical;
    bool fade;
    int32_t incoming_start;
    int32_t incoming_end;
    int32_t outgoing_start;
    int32_t outgoing_end;
} pm_anim_values_t;

struct pm_manager {
    lv_obj_t * root;
    lv_style_t * root_style;
    pm_page_t * pool;
    pm_page_t * current;
    pm_page_t * previous;
    pm_page_t * outgoing;
    size_t depth;
    pm_page_factory_t factory;
    pm_anim_config_t animation;
    pm_anim_config_t transition_animation;
    pm_anim_values_t values;
    pm_motion_t motion;
    bool busy;
    bool in_operation;
    unsigned int callback_depth;
    bool force_unload;
    bool drag_enabled;
    bool dragging;
    bool drag_moved;
    uint8_t drag_threshold;
    int32_t drag_extent;
    int32_t drag_offset;
    int32_t drag_velocity;
    pm_page_t * drag_below;
    lv_indev_t * drag_indev;
};

void pm_call(pm_page_t * page, pm_page_cb_t callback);
void pm_unload(pm_page_t * page);
void pm_finish_switch(pm_manager_t * manager);
bool pm_prepare_root(pm_page_t * page);
bool pm_anim_type_valid(pm_anim_type_t type, bool allow_global);
bool pm_anim_is_over(pm_anim_type_t type);
int32_t pm_anim_extent(pm_manager_t * manager, pm_anim_type_t type);
void pm_anim_configure(pm_manager_t * manager, bool entering, bool from_drag);
void pm_anim_start_switch(pm_manager_t * manager);
void pm_anim_stop(pm_manager_t * manager);
void pm_anim_reset_drag(pm_manager_t * manager);
void pm_drag_event(lv_event_t * event);
void pm_drag_finish(pm_manager_t * manager);
bool pm_pop_internal(pm_manager_t * manager, bool from_drag);

#endif
