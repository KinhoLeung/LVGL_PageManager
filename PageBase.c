/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#include "PM_Private.h"

lv_obj_t * pm_page_get_root(const pm_page_t * page) { return page ? page->root : NULL; }
pm_manager_t * pm_page_get_manager(const pm_page_t * page) { return page ? page->manager : NULL; }
const char * pm_page_get_name(const pm_page_t * page) { return page ? page->name : NULL; }
pm_page_state_t pm_page_get_state(const pm_page_t * page) { return page ? page->state : PM_PAGE_IDLE; }
void * pm_page_get_user_data(const pm_page_t * page) { return page ? page->user_data : NULL; }
void pm_page_set_user_data(pm_page_t * page, void * data) { if(page) page->user_data = data; }
uint32_t pm_page_get_id(const pm_page_t * page) { return page ? page->id : 0; }
void pm_page_set_id(pm_page_t * page, uint32_t id) { if(page) page->id = id; }
pm_cache_mode_t pm_page_get_cache_mode(const pm_page_t * page) { return page ? page->cache_mode : PM_CACHE_AUTO; }
bool pm_page_is_loaded(const pm_page_t * page) { return page && page->root; }
bool pm_page_is_in_stack(const pm_page_t * page) { return page && page->in_stack; }

bool pm_page_set_cache_mode(pm_page_t * page, pm_cache_mode_t mode)
{
    if(!page || mode < PM_CACHE_AUTO || mode > PM_CACHE_NONE) return false;
    page->cache_mode = mode;
    return true;
}

bool pm_page_set_animation(pm_page_t * page, pm_anim_type_t type,
                           uint32_t duration_ms, lv_anim_path_cb_t path)
{
    if(!page || !pm_anim_type_valid(type, true) || duration_ms > INT32_MAX) return false;
    page->animation.type = type;
    page->animation.duration_ms = duration_ms;
    page->animation.path = path ? path : lv_anim_path_ease_out;
    return true;
}

pm_anim_config_t pm_page_get_animation(const pm_page_t * page)
{
    pm_anim_config_t config = { PM_ANIM_GLOBAL, PM_ANIM_DURATION_DEFAULT, lv_anim_path_ease_out };
    return page ? page->animation : config;
}

void pm_page_stash_clear(pm_page_t * page)
{
    if(!page) return;
    lv_free(page->stash_data);
    page->stash_data = NULL;
    page->stash_size = 0;
}

bool pm_page_stash_pop(pm_page_t * page, void * destination, size_t size)
{
    if(!page || !destination || !page->stash_data || size != page->stash_size) return false;
    memcpy(destination, page->stash_data, size);
    pm_page_stash_clear(page);
    return true;
}

const void * pm_page_stash_peek(const pm_page_t * page, size_t * size)
{
    if(size) *size = page ? page->stash_size : 0;
    return page ? page->stash_data : NULL;
}
