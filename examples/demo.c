#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Route pm_log.h macros to the console for the demo */
#define PM_LOG_INFO(...)  do { printf("[INFO] " __VA_ARGS__); printf("\n"); } while (0)
#define PM_LOG_WARN(...)  do { printf("[WARN] " __VA_ARGS__); printf("\n"); } while (0)
#define PM_LOG_ERROR(...) do { printf("[ERROR] " __VA_ARGS__); printf("\n"); } while (0)

#include "page_manager.h"
#include "page_base.h"
#include "pm_factory.h"

/* Payload passed from Home -> Settings via pm_manager_push */
typedef struct
{
    uint8_t brightness;
} settings_payload_t;

/* --- Home page --------------------------------------------------------- */
static void home_on_view_load(pm_page_t *page)
{
    PM_LOG_INFO("Home: view loaded (root=%p)", (void *)page->root);
    /* Build your LVGL widgets here, e.g.
       lv_obj_t *label = lv_label_create(page->root);
       lv_label_set_text(label, "Home");
    */
}

static void home_on_view_did_appear(pm_page_t *page)
{
    PM_LOG_INFO("Home: presented on screen");
}

static const pm_page_vtable_t HOME_VTABLE = {
    .onCustomAttrConfig = NULL,
    .onViewLoad = home_on_view_load,
    .onViewDidLoad = NULL,
    .onViewWillAppear = NULL,
    .onViewDidAppear = home_on_view_did_appear,
    .onViewWillDisappear = NULL,
    .onViewDidDisappear = NULL,
    .onViewUnload = NULL,
    .onViewDidUnload = NULL,
};

static pm_page_t *home_page_create(void)
{
    return pm_page_create(&HOME_VTABLE);
}

/* --- Settings page ----------------------------------------------------- */
static void settings_on_custom_attr(pm_page_t *page)
{
    /* Keep the page cached and use a slide animation when entering */
    pm_page_set_custom_cache_enable(page, true);
    pm_page_set_custom_load_anim(page, PM_LOAD_ANIM_MOVE_LEFT, 300, NULL);
}

static void settings_on_view_load(pm_page_t *page)
{
    PM_LOG_INFO("Settings: view loaded (root=%p)", (void *)page->root);
}

static void settings_on_view_will_appear(pm_page_t *page)
{
    settings_payload_t payload;
    if (pm_page_stash_pop(page, &payload, sizeof(payload))) {
        PM_LOG_INFO("Settings: received brightness=%u from previous page", payload.brightness);
    } else {
        PM_LOG_WARN("Settings: no payload supplied");
    }
}

static void settings_on_view_did_disappear(pm_page_t *page)
{
    PM_LOG_INFO("Settings: leaving screen");
}

static const pm_page_vtable_t SETTINGS_VTABLE = {
    .onCustomAttrConfig = settings_on_custom_attr,
    .onViewLoad = settings_on_view_load,
    .onViewDidLoad = NULL,
    .onViewWillAppear = settings_on_view_will_appear,
    .onViewDidAppear = NULL,
    .onViewWillDisappear = NULL,
    .onViewDidDisappear = settings_on_view_did_disappear,
    .onViewUnload = NULL,
    .onViewDidUnload = NULL,
};

static pm_page_t *settings_page_create(void)
{
    return pm_page_create(&SETTINGS_VTABLE);
}

/* --- Factory and navigation ------------------------------------------- */
static pm_page_t *demo_factory_create(const char *class_name, void *user_ctx)
{
    (void)user_ctx;
    if (strcmp(class_name, "HomePage") == 0) return home_page_create();
    if (strcmp(class_name, "SettingsPage") == 0) return settings_page_create();
    PM_LOG_ERROR("Factory: unknown class %s", class_name);
    return NULL;
}

static void run_navigation_demo(pm_manager_t *manager)
{
    /* Install page classes so the manager can instantiate them on demand */
    pm_manager_install(manager, "HomePage", "home");
    pm_manager_install(manager, "SettingsPage", "settings");

    /* Push the home page as the first screen */
    pm_manager_push(manager, "home", NULL);

    /* Navigate to settings with a payload */
    settings_payload_t payload = { .brightness = 80 };
    pm_stash_t stash = PM_PAGE_STASH_MAKE(payload);
    pm_manager_push(manager, "settings", &stash);

    /* Go back to the previous page */
    pm_manager_pop(manager);

    /* Jump home (no-op here because we're already on home) */
    pm_manager_back_home(manager);
}

int main(void)
{
    /* In a real project you would initialize LVGL (lv_init, display, input, etc.) here. */

    const pm_factory_t factory = {
        .create = demo_factory_create,
        .destroy = NULL, /* fall back to pm_page_destroy */
        .user = NULL,
    };

    pm_manager_t *manager = pm_manager_create(&factory);
    if (!manager) {
        PM_LOG_ERROR("Failed to allocate PageManager");
        return -1;
    }

    /* Optional: change the default animation for all pages */
    pm_manager_set_global_load_anim(manager, PM_LOAD_ANIM_OVER_LEFT, 400, NULL);

    run_navigation_demo(manager);

    /* In production the manager lives for the app lifetime; destroy it during shutdown. */
    pm_manager_destroy(manager);
    return 0;
}

