/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#ifndef LVGL_PM_PAGE_MANAGER_H
#define LVGL_PM_PAGE_MANAGER_H

#include "PageBase.h"
#include "PageFactory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* All calls must run in the LVGL thread (or under the same LVGL lock).
 * parent == NULL selects lv_screen_active(). The manager owns a full-size
 * container and all registered page handles, names, roots and stash buffers.
 * Delete the manager BEFORE deleting its parent/display or calling lv_deinit.
 */
pm_manager_t * pm_manager_create(lv_obj_t * parent);
/* Can stop an in-flight animation. Returns false inside lifecycle callbacks. */
bool pm_manager_delete(pm_manager_t * manager);

pm_page_t * pm_manager_register(pm_manager_t * manager, const char * name,
                                const pm_page_descriptor_t * descriptor);
/* Refuses pages in the stack. Unloads any retained root before releasing it. */
bool pm_manager_unregister(pm_manager_t * manager, const char * name);
bool pm_manager_set_factory(pm_manager_t * manager, const pm_page_factory_t * factory);
pm_page_t * pm_manager_install(pm_manager_t * manager, const char * class_name,
                               const char * instance_name);
bool pm_manager_uninstall(pm_manager_t * manager, const char * name);

/* Names are unique, including within the stack. Requests while busy fail.
 * push/replace copy stash before changing the stack; NULL clears old stash.
 * replace requires a current page and always unloads the replaced page.
 * pop keeps the home page; back_home unloads every removed page.
 */
bool pm_manager_push(pm_manager_t * manager, const char * name, const pm_stash_t * stash);
bool pm_manager_replace(pm_manager_t * manager, const char * name, const pm_stash_t * stash);
bool pm_manager_pop(pm_manager_t * manager);
bool pm_manager_back_home(pm_manager_t * manager);
/* Clears the stack and every cached root; registrations remain available. */
bool pm_manager_clear(pm_manager_t * manager);

pm_page_t * pm_manager_find(const pm_manager_t * manager, const char * name);
pm_page_t * pm_manager_get_current(const pm_manager_t * manager);
/* Page below the stack top, NULL at home. */
pm_page_t * pm_manager_get_previous(const pm_manager_t * manager);
/* Source of the last/current transition, NULL until the second navigation. */
const char * pm_manager_get_previous_name(const pm_manager_t * manager);
size_t pm_manager_get_depth(const pm_manager_t * manager);
bool pm_manager_is_busy(const pm_manager_t * manager);
lv_obj_t * pm_manager_get_root(const pm_manager_t * manager);

bool pm_manager_set_animation(pm_manager_t * manager, pm_anim_type_t type,
                              uint32_t duration_ms, lv_anim_path_cb_t path);
/* Style is borrowed and applied to subsequently created roots. */
bool pm_manager_set_root_style(pm_manager_t * manager, lv_style_t * style);
/* Drag back supports OVER_* with a loaded page below. Enabled by default.
 * threshold_percent: 1..100 (default 50); inertia is included on release.
 */
bool pm_manager_set_drag(pm_manager_t * manager, bool enabled, uint8_t threshold_percent);

#ifdef __cplusplus
}
#endif
#endif
