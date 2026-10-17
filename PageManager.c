/* SPDX-License-Identifier: MIT
 * Based on X-TRACK PageManager, Copyright (c) 2021 _VIFEXTech.
 */
#include "PM_Private.h"

static bool ready(const pm_manager_t * manager)
{
    return manager && !manager->busy && !manager->in_operation;
}

void pm_call(pm_page_t * page, pm_page_cb_t callback)
{
    if(callback) {
        page->manager->callback_depth++;
        callback(page);
        page->manager->callback_depth--;
    }
}

pm_manager_t * pm_manager_create(lv_obj_t * parent)
{
    pm_manager_t * manager;
    if(!parent) parent = lv_screen_active();
    if(!parent) return NULL;
    manager = lv_malloc(sizeof(*manager));
    if(!manager) return NULL;
    memset(manager, 0, sizeof(*manager));
    manager->root = lv_obj_create(parent);
    if(!manager->root) {
        lv_free(manager);
        return NULL;
    }
    lv_obj_remove_style_all(manager->root);
    lv_obj_set_size(manager->root, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(manager->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(manager->root, LV_OBJ_FLAG_CLICKABLE);
    /* Pages are positioned by the manager even when the parent uses a layout. */
    lv_obj_add_flag(manager->root, LV_OBJ_FLAG_IGNORE_LAYOUT);
    manager->animation.type = PM_ANIM_OVER_LEFT;
    manager->animation.duration_ms = PM_ANIM_DURATION_DEFAULT;
    manager->animation.path = lv_anim_path_ease_out;
    manager->drag_enabled = true;
    manager->drag_threshold = 50;
    return manager;
}

pm_page_t * pm_manager_find(const pm_manager_t * manager, const char * name)
{
    pm_page_t * page;
    if(!manager || !name) return NULL;
    for(page = manager->pool; page; page = page->pool_next) {
        if(strcmp(page->name, name) == 0) return page;
    }
    return NULL;
}

static pm_page_t * register_page(pm_manager_t * manager, const char * name,
                                 const pm_page_descriptor_t * descriptor)
{
    size_t length = strlen(name) + 1;
    pm_page_t * page = lv_malloc(sizeof(*page));
    if(!page) return NULL;
    memset(page, 0, sizeof(*page));
    page->name = lv_malloc(length);
    if(!page->name) {
        lv_free(page);
        return NULL;
    }
    memcpy(page->name, name, length);
    page->manager = manager;
    page->callbacks = descriptor->callbacks;
    page->user_data = descriptor->user_data;
    page->animation.type = PM_ANIM_GLOBAL;
    page->animation.duration_ms = PM_ANIM_DURATION_DEFAULT;
    page->animation.path = lv_anim_path_ease_out;
    page->pool_next = manager->pool;
    manager->pool = page;
    pm_call(page, page->callbacks.on_custom_attr_config);
    return page;
}

pm_page_t * pm_manager_register(pm_manager_t * manager, const char * name,
                                const pm_page_descriptor_t * descriptor)
{
    pm_page_t * page;
    if(!ready(manager) || !name || !name[0] || !descriptor || pm_manager_find(manager, name)) return NULL;
    manager->in_operation = true;
    manager->busy = true;
    page = register_page(manager, name, descriptor);
    manager->busy = false;
    manager->in_operation = false;
    return page;
}

bool pm_manager_set_factory(pm_manager_t * manager, const pm_page_factory_t * factory)
{
    if(!ready(manager)) return false;
    if(factory) manager->factory = *factory;
    else memset(&manager->factory, 0, sizeof(manager->factory));
    return true;
}

pm_page_t * pm_manager_install(pm_manager_t * manager, const char * class_name,
                               const char * instance_name)
{
    pm_page_descriptor_t descriptor;
    pm_page_t * page = NULL;
    if(!ready(manager) || !class_name || !class_name[0] || !manager->factory.create) return NULL;
    if(!instance_name) instance_name = class_name;
    if(!instance_name[0] || pm_manager_find(manager, instance_name)) return NULL;
    memset(&descriptor, 0, sizeof(descriptor));
    manager->in_operation = true;
    manager->busy = true;
    if(manager->factory.create(class_name, instance_name, &descriptor, manager->factory.user_data)) {
        page = register_page(manager, instance_name, &descriptor);
        if(page) {
            page->factory_destroy = manager->factory.destroy;
            page->factory_user_data = manager->factory.user_data;
        }
        else if(manager->factory.destroy) {
            manager->factory.destroy(descriptor.user_data, manager->factory.user_data);
        }
    }
    manager->busy = false;
    manager->in_operation = false;
    return page;
}

bool pm_prepare_root(pm_page_t * page)
{
    lv_obj_t * root;
    if(page->root) return true;
    root = lv_obj_create(page->manager->root);
    if(!root) return false;
    page->root = root;
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(root, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(root, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_radius(root, 0, LV_PART_MAIN);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    if(page->manager->root_style) lv_obj_add_style(root, page->manager->root_style, LV_PART_MAIN);
    lv_obj_add_event_cb(root, pm_drag_event, LV_EVENT_ALL, page);
    return true;
}

void pm_unload(pm_page_t * page)
{
    if(page->root) {
        page->state = PM_PAGE_UNLOADING;
        pm_call(page, page->callbacks.on_view_unload);
        /* The switch animation targets the manager, so deleting a page root
         * cannot invalidate an animation descriptor being completed by LVGL. */
        lv_obj_delete(page->root);
        page->root = NULL;
        pm_page_stash_clear(page);
        page->state = PM_PAGE_IDLE;
        pm_call(page, page->callbacks.on_view_did_unload);
    }
    else {
        pm_page_stash_clear(page);
        page->state = PM_PAGE_IDLE;
    }
}

static void release_page(pm_page_t * page)
{
    pm_unload(page);
    if(page->factory_destroy) page->factory_destroy(page->user_data, page->factory_user_data);
    lv_free(page->name);
    lv_free(page);
}

bool pm_manager_unregister(pm_manager_t * manager, const char * name)
{
    pm_page_t * page;
    pm_page_t ** link;
    if(!ready(manager)) return false;
    page = pm_manager_find(manager, name);
    if(!page || page->in_stack) return false;
    manager->in_operation = true;
    manager->busy = true;
    for(link = &manager->pool; *link != page; link = &(*link)->pool_next) { }
    *link = page->pool_next;
    if(manager->previous == page) manager->previous = NULL;
    release_page(page);
    manager->busy = false;
    manager->in_operation = false;
    return true;
}

bool pm_manager_uninstall(pm_manager_t * manager, const char * name)
{
    return pm_manager_unregister(manager, name);
}

static void load_view(pm_page_t * page)
{
    if(page->state != PM_PAGE_IDLE) return;
    page->state = PM_PAGE_LOADING;
    pm_call(page, page->callbacks.on_view_load);
    pm_call(page, page->callbacks.on_view_did_load);
}

static void begin_switch(pm_manager_t * manager, pm_page_t * incoming,
                         pm_page_t * outgoing, bool entering, bool force_unload,
                         bool from_drag)
{
    manager->outgoing = outgoing;
    manager->previous = outgoing;
    manager->current = incoming;
    manager->force_unload = force_unload;
    if(entering) {
        incoming->entry_animation = incoming->animation.type == PM_ANIM_GLOBAL ?
                                    manager->animation : incoming->animation;
        manager->transition_animation = incoming->entry_animation;
    }
    else {
        manager->transition_animation = outgoing->entry_animation;
    }
    if(outgoing) {
        outgoing->state = PM_PAGE_WILL_DISAPPEAR;
        pm_call(outgoing, outgoing->callbacks.on_view_will_disappear);
    }
    load_view(incoming);
    incoming->state = PM_PAGE_WILL_APPEAR;
    pm_call(incoming, incoming->callbacks.on_view_will_appear);
    lv_obj_remove_flag(incoming->root, LV_OBJ_FLAG_HIDDEN);
    if(entering) {
        if(outgoing) lv_obj_move_to_index(outgoing->root, -1);
        lv_obj_move_to_index(incoming->root, -1);
    }
    else {
        lv_obj_move_to_index(incoming->root, -1);
        lv_obj_move_to_index(outgoing->root, -1);
    }
    pm_anim_configure(manager, entering, from_drag);
    pm_anim_start_switch(manager);
}

void pm_finish_switch(pm_manager_t * manager)
{
    pm_page_t * outgoing = manager->outgoing;
    pm_page_t * incoming = manager->current;
    bool was_in_operation = manager->in_operation;
    manager->in_operation = true;
    manager->motion = PM_MOTION_NONE;
    if(outgoing) {
        lv_obj_add_flag(outgoing->root, LV_OBJ_FLAG_HIDDEN);
        outgoing->state = PM_PAGE_CACHED;
        pm_call(outgoing, outgoing->callbacks.on_view_did_disappear);
        if(manager->force_unload || outgoing->cache_mode == PM_CACHE_NONE ||
           (outgoing->cache_mode == PM_CACHE_AUTO && !outgoing->in_stack)) {
            pm_unload(outgoing);
        }
    }
    /* Reset reserved presentation properties so a retained root can be reused
     * with a different animation, including fade -> move and move -> none. */
    lv_obj_set_pos(incoming->root, 0, 0);
    lv_obj_set_style_opa(incoming->root, LV_OPA_COVER, LV_PART_MAIN);
    incoming->state = PM_PAGE_ACTIVE;
    pm_call(incoming, incoming->callbacks.on_view_did_appear);
    manager->outgoing = NULL;
    manager->force_unload = false;
    manager->busy = false;
    manager->in_operation = was_in_operation;
}

static bool navigate_to(pm_manager_t * manager, const char * name,
                        const pm_stash_t * stash, bool replace)
{
    pm_page_t * incoming;
    pm_page_t * outgoing;
    void * stash_copy = NULL;
    size_t stash_size = 0;
    if(!ready(manager)) return false;
    incoming = pm_manager_find(manager, name);
    if(!incoming || incoming->in_stack || (replace && !manager->current)) return false;
    if(stash && (!stash->data || !stash->size)) return false;
    manager->in_operation = true;
    manager->busy = true;
    if(stash) {
        stash_size = stash->size;
        stash_copy = lv_malloc(stash_size);
        if(!stash_copy) goto fail;
        memcpy(stash_copy, stash->data, stash_size);
    }
    if(!pm_prepare_root(incoming)) goto fail;
    pm_page_stash_clear(incoming);
    incoming->stash_data = stash_copy;
    incoming->stash_size = stash_size;
    outgoing = manager->current;
    if(replace) {
        incoming->stack_previous = outgoing->stack_previous;
        outgoing->in_stack = false;
        outgoing->stack_previous = NULL;
    }
    else {
        incoming->stack_previous = outgoing;
        manager->depth++;
    }
    incoming->in_stack = true;
    begin_switch(manager, incoming, outgoing, true, replace, false);
    manager->in_operation = false;
    return true;
fail:
    lv_free(stash_copy);
    manager->busy = false;
    manager->in_operation = false;
    return false;
}

bool pm_manager_push(pm_manager_t * manager, const char * name, const pm_stash_t * stash)
{
    return navigate_to(manager, name, stash, false);
}

bool pm_manager_replace(pm_manager_t * manager, const char * name, const pm_stash_t * stash)
{
    return navigate_to(manager, name, stash, true);
}

bool pm_pop_internal(pm_manager_t * manager, bool from_drag)
{
    pm_page_t * outgoing;
    pm_page_t * incoming;
    if(!manager || manager->in_operation || (manager->busy && !from_drag) || manager->depth <= 1) return false;
    outgoing = manager->current;
    incoming = outgoing->stack_previous;
    manager->in_operation = true;
    manager->busy = true;
    if(!pm_prepare_root(incoming)) {
        manager->busy = from_drag;
        manager->in_operation = false;
        return false;
    }
    manager->depth--;
    outgoing->in_stack = false;
    outgoing->stack_previous = NULL;
    begin_switch(manager, incoming, outgoing, false, false, from_drag);
    manager->in_operation = false;
    return true;
}

bool pm_manager_pop(pm_manager_t * manager)
{
    return pm_pop_internal(manager, false);
}

bool pm_manager_back_home(pm_manager_t * manager)
{
    pm_page_t * home;
    pm_page_t * outgoing;
    pm_page_t * page;
    if(!ready(manager) || manager->depth <= 1) return false;
    outgoing = manager->current;
    home = outgoing;
    while(home->stack_previous) home = home->stack_previous;
    manager->in_operation = true;
    manager->busy = true;
    if(!pm_prepare_root(home)) {
        manager->busy = false;
        manager->in_operation = false;
        return false;
    }
    page = outgoing->stack_previous;
    while(page != home) {
        pm_page_t * previous = page->stack_previous;
        page->in_stack = false;
        page->stack_previous = NULL;
        pm_unload(page);
        page = previous;
    }
    outgoing->in_stack = false;
    outgoing->stack_previous = NULL;
    manager->depth = 1;
    begin_switch(manager, home, outgoing, false, true, false);
    manager->in_operation = false;
    return true;
}

static void clear_pages(pm_manager_t * manager)
{
    pm_page_t * page;
    /* Finish the accepted transition before ending the active lifecycle. */
    if(manager->motion == PM_MOTION_SWITCH) {
        pm_anim_stop(manager);
        pm_finish_switch(manager);
        manager->busy = true;
    }
    else {
        pm_anim_stop(manager);
        pm_drag_finish(manager);
        manager->busy = true;
    }
    if(manager->current) {
        page = manager->current;
        page->state = PM_PAGE_WILL_DISAPPEAR;
        pm_call(page, page->callbacks.on_view_will_disappear);
        lv_obj_add_flag(page->root, LV_OBJ_FLAG_HIDDEN);
        page->state = PM_PAGE_CACHED;
        pm_call(page, page->callbacks.on_view_did_disappear);
    }
    manager->current = NULL;
    manager->previous = NULL;
    manager->outgoing = NULL;
    manager->depth = 0;
    for(page = manager->pool; page; page = page->pool_next) {
        page->in_stack = false;
        page->stack_previous = NULL;
        pm_unload(page);
    }
}

bool pm_manager_clear(pm_manager_t * manager)
{
    if(!manager || manager->callback_depth || manager->in_operation) return false;
    manager->in_operation = true;
    manager->busy = true;
    clear_pages(manager);
    manager->busy = false;
    manager->in_operation = false;
    return true;
}

bool pm_manager_delete(pm_manager_t * manager)
{
    pm_page_t * page;
    if(!manager || manager->callback_depth || manager->in_operation) return false;
    manager->in_operation = true;
    manager->busy = true;
    clear_pages(manager);
    while(manager->pool) {
        page = manager->pool;
        manager->pool = page->pool_next;
        release_page(page);
    }
    lv_obj_delete(manager->root);
    lv_free(manager);
    return true;
}

pm_page_t * pm_manager_get_current(const pm_manager_t * manager) { return manager ? manager->current : NULL; }
pm_page_t * pm_manager_get_previous(const pm_manager_t * manager)
{
    return manager && manager->current ? manager->current->stack_previous : NULL;
}
const char * pm_manager_get_previous_name(const pm_manager_t * manager)
{
    return manager && manager->previous ? manager->previous->name : NULL;
}
size_t pm_manager_get_depth(const pm_manager_t * manager) { return manager ? manager->depth : 0; }
bool pm_manager_is_busy(const pm_manager_t * manager) { return manager && (manager->busy || manager->in_operation); }
lv_obj_t * pm_manager_get_root(const pm_manager_t * manager) { return manager ? manager->root : NULL; }

bool pm_manager_set_animation(pm_manager_t * manager, pm_anim_type_t type,
                              uint32_t duration_ms, lv_anim_path_cb_t path)
{
    if(!ready(manager) || !pm_anim_type_valid(type, false) || duration_ms > INT32_MAX) return false;
    manager->animation.type = type;
    manager->animation.duration_ms = duration_ms;
    manager->animation.path = path ? path : lv_anim_path_ease_out;
    return true;
}

bool pm_manager_set_root_style(pm_manager_t * manager, lv_style_t * style)
{
    if(!ready(manager)) return false;
    manager->root_style = style;
    return true;
}

bool pm_manager_set_drag(pm_manager_t * manager, bool enabled, uint8_t threshold_percent)
{
    if(!ready(manager) || threshold_percent < 1 || threshold_percent > 100) return false;
    manager->drag_enabled = enabled;
    manager->drag_threshold = threshold_percent;
    return true;
}
