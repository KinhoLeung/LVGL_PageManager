/* SPDX-License-Identifier: MIT */
#include "page_manager_example.h"

typedef struct {
    uint32_t visit;
} details_parameters_t;

static void open_details(lv_event_t * event)
{
    pm_page_t * page = lv_event_get_user_data(event);
    details_parameters_t parameters = { pm_page_get_id(page) + 1 };
    pm_stash_t stash = PM_STASH_MAKE(parameters);
    if(pm_manager_push(pm_page_get_manager(page), "details", &stash)) {
        pm_page_set_id(page, parameters.visit);
    }
}

static void go_back(lv_event_t * event)
{
    pm_page_t * page = lv_event_get_user_data(event);
    (void)pm_manager_pop(pm_page_get_manager(page));
}

static lv_obj_t * create_button(lv_obj_t * parent, const char * text,
                                lv_event_cb_t callback, pm_page_t * page)
{
    lv_obj_t * button = lv_button_create(parent);
    lv_obj_t * label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, page);
    return button;
}

static void home_load(pm_page_t * page)
{
    lv_obj_t * root = pm_page_get_root(page);
    lv_obj_t * title = lv_label_create(root);
    lv_label_set_text(title, "C PageManager / LVGL 9");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_center(create_button(root, "Open details", open_details, page));
}

static void details_load(pm_page_t * page)
{
    lv_obj_t * root = pm_page_get_root(page);
    lv_obj_t * label = lv_label_create(root);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_center(create_button(root, "Back", go_back, page));
}

static void details_will_appear(pm_page_t * page)
{
    details_parameters_t parameters = { 0 };
    (void)pm_page_stash_pop(page, &parameters, sizeof(parameters));
    lv_label_set_text_fmt(lv_obj_get_child(pm_page_get_root(page), 0),
                          "Visit %lu\nDrag the background right to go back",
                          (unsigned long)parameters.visit);
}

pm_manager_t * pm_example_create(lv_obj_t * parent)
{
    pm_page_descriptor_t home = { .callbacks = { .on_view_load = home_load } };
    pm_page_descriptor_t details = {
        .callbacks = { .on_view_load = details_load, .on_view_will_appear = details_will_appear }
    };
    pm_manager_t * manager = pm_manager_create(parent);
    if(!manager) return NULL;
    (void)pm_manager_set_animation(manager, PM_ANIM_OVER_LEFT, 250, lv_anim_path_ease_out);
    if(!pm_manager_register(manager, "home", &home) ||
       !pm_manager_register(manager, "details", &details) ||
       !pm_manager_push(manager, "home", NULL)) {
        (void)pm_manager_delete(manager);
        return NULL;
    }
    return manager;
}
