/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#ifndef LVGL_PM_PAGE_BASE_H
#define LVGL_PM_PAGE_BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Override with e.g. -DPM_LVGL_INCLUDE=\"lvgl/lvgl.h\" if necessary. */
#ifndef PM_LVGL_INCLUDE
#define PM_LVGL_INCLUDE "lvgl.h"
#endif
#include PM_LVGL_INCLUDE

#if LVGL_VERSION_MAJOR != 9
#error "LVGL_PageManager requires LVGL 9.x"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pm_manager pm_manager_t;
typedef struct pm_page pm_page_t;

typedef enum {
    PM_ANIM_GLOBAL = 0,
    PM_ANIM_OVER_LEFT,
    PM_ANIM_OVER_RIGHT,
    PM_ANIM_OVER_TOP,
    PM_ANIM_OVER_BOTTOM,
    PM_ANIM_MOVE_LEFT,
    PM_ANIM_MOVE_RIGHT,
    PM_ANIM_MOVE_TOP,
    PM_ANIM_MOVE_BOTTOM,
    PM_ANIM_FADE_ON,
    PM_ANIM_NONE
} pm_anim_type_t;

typedef struct {
    pm_anim_type_t type;
    uint32_t duration_ms;
    lv_anim_path_cb_t path;
} pm_anim_config_t;

typedef enum {
    PM_PAGE_IDLE = 0,
    PM_PAGE_LOADING,
    PM_PAGE_WILL_APPEAR,
    PM_PAGE_ACTIVE,
    PM_PAGE_WILL_DISAPPEAR,
    PM_PAGE_CACHED,
    PM_PAGE_UNLOADING
} pm_page_state_t;

typedef enum {
    PM_CACHE_AUTO = 0, /* Keep while in the navigation stack; unload on pop. */
    PM_CACHE_KEEP,     /* Also keep after pop, until clear/unregister/delete. */
    PM_CACHE_NONE      /* Unload whenever the page disappears. */
} pm_cache_mode_t;

typedef struct {
    const void * data;
    size_t size;
} pm_stash_t;

#define PM_STASH_MAKE(value) { &(value), sizeof(value) }
#define PM_ANIM_DURATION_DEFAULT 500U

typedef void (*pm_page_cb_t)(pm_page_t * page);

typedef struct {
    pm_page_cb_t on_custom_attr_config;
    pm_page_cb_t on_view_load;
    pm_page_cb_t on_view_did_load;
    pm_page_cb_t on_view_will_appear;
    pm_page_cb_t on_view_did_appear;
    pm_page_cb_t on_view_will_disappear;
    pm_page_cb_t on_view_did_disappear;
    pm_page_cb_t on_view_unload;
    pm_page_cb_t on_view_did_unload;
} pm_page_callbacks_t;

/* Copied at registration. user_data remains owned by the application. */
typedef struct {
    pm_page_callbacks_t callbacks;
    void * user_data;
} pm_page_descriptor_t;

lv_obj_t * pm_page_get_root(const pm_page_t * page);
pm_manager_t * pm_page_get_manager(const pm_page_t * page);
const char * pm_page_get_name(const pm_page_t * page);
pm_page_state_t pm_page_get_state(const pm_page_t * page);
void * pm_page_get_user_data(const pm_page_t * page);
void pm_page_set_user_data(pm_page_t * page, void * user_data);
uint32_t pm_page_get_id(const pm_page_t * page);
void pm_page_set_id(pm_page_t * page, uint32_t id);

bool pm_page_set_cache_mode(pm_page_t * page, pm_cache_mode_t mode);
pm_cache_mode_t pm_page_get_cache_mode(const pm_page_t * page);
bool pm_page_is_loaded(const pm_page_t * page);
bool pm_page_is_in_stack(const pm_page_t * page);

/* GLOBAL inherits the manager settings. A NULL path uses ease_out. */
bool pm_page_set_animation(pm_page_t * page, pm_anim_type_t type,
                           uint32_t duration_ms, lv_anim_path_cb_t path);
pm_anim_config_t pm_page_get_animation(const pm_page_t * page);

/* Exact-size copy and consume. Failed reads leave the stash intact. */
bool pm_page_stash_pop(pm_page_t * page, void * destination, size_t size);
const void * pm_page_stash_peek(const pm_page_t * page, size_t * size);
void pm_page_stash_clear(pm_page_t * page);

#ifdef __cplusplus
}
#endif
#endif
