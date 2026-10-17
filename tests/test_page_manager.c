/* SPDX-License-Identifier: MIT
 * Integration tests against real LVGL objects, animation timers and input.
 */
#include "PageManager.h"
#include "../examples/page_manager_example.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if(!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

enum { CONFIG, LOAD, DID_LOAD, WILL_APPEAR, DID_APPEAR, WILL_DISAPPEAR, DID_DISAPPEAR, UNLOAD, DID_UNLOAD };
typedef struct { unsigned int calls[9]; } page_context_t;
typedef struct {
    pm_manager_t * manager;
    pm_page_t * pages[4];
    page_context_t contexts[4];
} fixture_t;

static lv_display_t * display;
static uint32_t pixels[320 * 16];
static char trace[8192];
static size_t trace_size;

static void record(pm_page_t * page, int kind, char token)
{
    pm_manager_t * manager = pm_page_get_manager(page);
    page_context_t * context = pm_page_get_user_data(page);
    int written;
    context->calls[kind]++;
    CHECK(pm_manager_is_busy(manager));
    /* Callback reentrancy must never mutate a half-committed stack. */
    CHECK(!pm_manager_pop(manager));
    CHECK(!pm_manager_clear(manager));
    CHECK(!pm_manager_delete(manager));
    if(trace_size + 16 >= sizeof(trace)) return;
    written = snprintf(trace + trace_size, sizeof(trace) - trace_size,
                       "%s:%c;", pm_page_get_name(page), token);
    CHECK(written > 0);
    trace_size += (size_t)written;
}

static void config(pm_page_t * page) { record(page, CONFIG, 'c'); }
static void load(pm_page_t * page)
{
    lv_obj_t * label;
    CHECK(pm_page_get_state(page) == PM_PAGE_LOADING);
    CHECK(pm_page_get_root(page));
    record(page, LOAD, 'L');
    label = lv_label_create(pm_page_get_root(page));
    lv_label_set_text(label, pm_page_get_name(page));
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);
}
static void did_load(pm_page_t * page) { record(page, DID_LOAD, 'l'); }
static void will_appear(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_WILL_APPEAR);
    CHECK(pm_manager_get_current(pm_page_get_manager(page)) == page);
    record(page, WILL_APPEAR, 'A');
}
static void did_appear(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_ACTIVE);
    CHECK(!lv_obj_has_flag(pm_page_get_root(page), LV_OBJ_FLAG_HIDDEN));
    record(page, DID_APPEAR, 'a');
}
static void will_disappear(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_WILL_DISAPPEAR);
    record(page, WILL_DISAPPEAR, 'D');
}
static void did_disappear(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_CACHED);
    CHECK(lv_obj_has_flag(pm_page_get_root(page), LV_OBJ_FLAG_HIDDEN));
    record(page, DID_DISAPPEAR, 'd');
}
static void unload(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_UNLOADING);
    CHECK(pm_page_get_root(page));
    record(page, UNLOAD, 'U');
}
static void did_unload(pm_page_t * page)
{
    CHECK(pm_page_get_state(page) == PM_PAGE_IDLE);
    CHECK(!pm_page_get_root(page));
    CHECK(!pm_page_stash_peek(page, NULL));
    record(page, DID_UNLOAD, 'u');
}

static const pm_page_callbacks_t callbacks = {
    .on_custom_attr_config = config,
    .on_view_load = load,
    .on_view_did_load = did_load,
    .on_view_will_appear = will_appear,
    .on_view_did_appear = did_appear,
    .on_view_will_disappear = will_disappear,
    .on_view_did_disappear = did_disappear,
    .on_view_unload = unload,
    .on_view_did_unload = did_unload
};

static void reset_trace(void) { trace[0] = '\0'; trace_size = 0; }
static void tick(uint32_t milliseconds)
{
    uint32_t elapsed;
    for(elapsed = 0; elapsed < milliseconds; elapsed += 10) {
        lv_tick_inc(10);
        (void)lv_timer_handler();
    }
}
static void settle(pm_manager_t * manager)
{
    unsigned int count = 0;
    while(pm_manager_is_busy(manager) && count++ < 200) tick(10);
    CHECK(!pm_manager_is_busy(manager));
    lv_obj_update_layout(pm_manager_get_root(manager));
}
static void flush(lv_display_t * target, const lv_area_t * area, uint8_t * buffer)
{
    (void)area;
    (void)buffer;
    lv_display_flush_ready(target);
}

static void fixture_init(fixture_t * fixture)
{
    static const char * names[] = { "A", "B", "C", "D" };
    unsigned int i;
    memset(fixture, 0, sizeof(*fixture));
    fixture->manager = pm_manager_create(NULL);
    CHECK(fixture->manager);
    CHECK(pm_manager_set_animation(fixture->manager, PM_ANIM_NONE, 0, NULL));
    for(i = 0; i < 4; i++) {
        pm_page_descriptor_t descriptor = { .callbacks = callbacks, .user_data = &fixture->contexts[i] };
        fixture->pages[i] = pm_manager_register(fixture->manager, names[i], &descriptor);
        CHECK(fixture->pages[i]);
        CHECK(fixture->contexts[i].calls[CONFIG] == 1);
    }
    reset_trace();
}

static void test_lifecycle_and_stash(void)
{
    fixture_t f;
    int value = 42;
    int received = 0;
    char larger[] = "larger payload";
    size_t size;
    pm_stash_t stash = PM_STASH_MAKE(value);
    pm_stash_t large_stash = PM_STASH_MAKE(larger);
    pm_stash_t invalid = { NULL, sizeof(value) };
    pm_stash_t too_large = { &value, 8U * 1024U * 1024U };
    pm_page_descriptor_t empty = { 0 };
    fixture_init(&f);
    CHECK(!pm_manager_register(f.manager, "A", &empty));
    CHECK(!pm_manager_register(f.manager, "", &empty));
    CHECK(!pm_manager_replace(f.manager, "A", NULL));
    CHECK(!pm_manager_pop(f.manager));
    CHECK(!pm_manager_back_home(f.manager));
    CHECK(!pm_manager_push(f.manager, "missing", NULL));
    /* The test LVGL heap is 4 MiB: allocation fails before the copy or commit. */
    CHECK(!pm_manager_push(f.manager, "A", &too_large));
    CHECK(pm_manager_get_depth(f.manager) == 0);
    CHECK(!pm_page_is_loaded(f.pages[0]));
    CHECK(!pm_manager_is_busy(f.manager));
    CHECK(pm_manager_push(f.manager, "A", NULL));
    CHECK(strcmp(trace, "A:L;A:l;A:A;A:a;") == 0);
    CHECK(pm_manager_get_depth(f.manager) == 1);
    CHECK(!pm_manager_get_previous_name(f.manager));
    CHECK(!pm_manager_push(f.manager, "A", NULL));
    CHECK(!pm_manager_unregister(f.manager, "A"));
    reset_trace();
    CHECK(pm_manager_push(f.manager, "B", &stash));
    CHECK(strcmp(trace, "A:D;B:L;B:l;B:A;A:d;B:a;") == 0);
    CHECK(pm_manager_get_previous(f.manager) == f.pages[0]);
    CHECK(strcmp(pm_manager_get_previous_name(f.manager), "A") == 0);
    value = 99;
    CHECK(*(const int *)pm_page_stash_peek(f.pages[1], &size) == 42);
    CHECK(size == sizeof(value));
    CHECK(!pm_page_stash_pop(f.pages[1], &received, sizeof(received) - 1));
    CHECK(pm_page_stash_pop(f.pages[1], &received, sizeof(received)));
    CHECK(received == 42);
    CHECK(!pm_page_stash_pop(f.pages[1], &received, sizeof(received)));
    CHECK(pm_page_is_loaded(f.pages[0]));
    reset_trace();
    CHECK(pm_manager_pop(f.manager));
    CHECK(strcmp(trace, "B:D;A:A;B:d;B:U;B:u;A:a;") == 0);
    CHECK(!pm_page_is_loaded(f.pages[1]));
    CHECK(f.contexts[0].calls[LOAD] == 1);
    CHECK(pm_page_set_cache_mode(f.pages[1], PM_CACHE_KEEP));
    CHECK(pm_manager_push(f.manager, "B", &stash));
    CHECK(pm_manager_pop(f.manager));
    CHECK(pm_page_is_loaded(f.pages[1]));
    CHECK(!pm_page_is_in_stack(f.pages[1]));
    CHECK(pm_manager_push(f.manager, "B", &large_stash));
    CHECK(strcmp(pm_page_stash_peek(f.pages[1], &size), larger) == 0);
    CHECK(size == sizeof(larger));
    CHECK(f.contexts[1].calls[LOAD] == 2);
    CHECK(pm_manager_pop(f.manager));
    CHECK(pm_manager_push(f.manager, "B", NULL));
    CHECK(!pm_page_stash_peek(f.pages[1], &size) && size == 0);
    CHECK(!pm_manager_replace(f.manager, "C", &invalid));
    CHECK(pm_manager_get_current(f.manager) == f.pages[1]);
    CHECK(pm_manager_get_depth(f.manager) == 2);
    CHECK(!pm_page_is_loaded(f.pages[2]));
    CHECK(pm_manager_replace(f.manager, "C", NULL));
    CHECK(!pm_page_is_loaded(f.pages[1])); /* replace overrides KEEP */
    CHECK(pm_manager_get_depth(f.manager) == 2);
    CHECK(pm_manager_get_previous(f.manager) == f.pages[0]);
    CHECK(pm_manager_push(f.manager, "D", NULL));
    CHECK(pm_page_set_cache_mode(f.pages[2], PM_CACHE_KEEP));
    CHECK(pm_manager_back_home(f.manager));
    CHECK(pm_manager_get_current(f.manager) == f.pages[0]);
    CHECK(pm_manager_get_depth(f.manager) == 1);
    CHECK(!pm_page_is_loaded(f.pages[2]) && !pm_page_is_loaded(f.pages[3]));
    CHECK(!pm_manager_back_home(f.manager));
    CHECK(pm_page_set_cache_mode(f.pages[0], PM_CACHE_NONE));
    CHECK(pm_manager_push(f.manager, "B", NULL));
    CHECK(!pm_page_is_loaded(f.pages[0]));
    CHECK(pm_manager_pop(f.manager));
    CHECK(f.contexts[0].calls[LOAD] == 2);
    CHECK(pm_manager_clear(f.manager));
    CHECK(!pm_manager_get_current(f.manager));
    CHECK(pm_manager_get_depth(f.manager) == 0);
    CHECK(pm_manager_push(f.manager, "D", NULL));
    CHECK(pm_manager_delete(f.manager));
}

static bool vertical(pm_anim_type_t type)
{
    return type == PM_ANIM_OVER_TOP || type == PM_ANIM_OVER_BOTTOM ||
           type == PM_ANIM_MOVE_TOP || type == PM_ANIM_MOVE_BOTTOM;
}
static bool reversed(pm_anim_type_t type)
{
    return type == PM_ANIM_OVER_RIGHT || type == PM_ANIM_OVER_BOTTOM ||
           type == PM_ANIM_MOVE_RIGHT || type == PM_ANIM_MOVE_BOTTOM;
}
static void test_animations(void)
{
    pm_anim_type_t type;
    for(type = PM_ANIM_OVER_LEFT; type <= PM_ANIM_NONE; type++) {
        fixture_t f;
        int32_t extent = vertical(type) ? 120 : 160;
        fixture_init(&f);
        lv_obj_set_size(pm_manager_get_root(f.manager), 160, 120);
        CHECK(pm_manager_push(f.manager, "A", NULL));
        CHECK(pm_page_set_animation(f.pages[1], type, 120, lv_anim_path_linear));
        CHECK(pm_manager_push(f.manager, "B", NULL));
        if(reversed(type)) extent = -extent;
        if(type != PM_ANIM_NONE) {
            CHECK(pm_manager_is_busy(f.manager));
            CHECK(!pm_manager_push(f.manager, "C", NULL));
            CHECK(!pm_manager_pop(f.manager));
            CHECK(!pm_manager_unregister(f.manager, "D"));
            CHECK(!pm_manager_set_animation(f.manager, PM_ANIM_NONE, 0, NULL));
            if(type == PM_ANIM_FADE_ON) {
                CHECK(lv_obj_get_style_opa(pm_page_get_root(f.pages[1]), LV_PART_MAIN) == LV_OPA_TRANSP);
            }
            else {
                lv_obj_update_layout(pm_manager_get_root(f.manager));
                CHECK((vertical(type) ? lv_obj_get_y(pm_page_get_root(f.pages[1])) :
                       lv_obj_get_x(pm_page_get_root(f.pages[1]))) == extent);
            }
        }
        settle(f.manager);
        CHECK(f.contexts[1].calls[DID_APPEAR] == 1);
        CHECK(lv_obj_get_x(pm_page_get_root(f.pages[1])) == 0);
        CHECK(lv_obj_get_y(pm_page_get_root(f.pages[1])) == 0);
        CHECK(lv_obj_get_style_opa(pm_page_get_root(f.pages[1]), LV_PART_MAIN) == LV_OPA_COVER);
        /* Pop must reverse the entry snapshot even if global/page settings change. */
        CHECK(pm_manager_set_animation(f.manager, PM_ANIM_NONE, 0, NULL));
        CHECK(pm_page_set_animation(f.pages[1], PM_ANIM_NONE, 0, NULL));
        CHECK(pm_manager_pop(f.manager));
        if(type != PM_ANIM_NONE) CHECK(pm_manager_is_busy(f.manager));
        settle(f.manager);
        CHECK(!pm_page_is_loaded(f.pages[1]));
        CHECK(f.contexts[0].calls[DID_APPEAR] == 2);
        CHECK(pm_manager_delete(f.manager));
    }
}

static void test_cancel_and_memory(void)
{
    fixture_t f;
    lv_mem_monitor_t before;
    lv_mem_monitor_t after;
    unsigned int i;
    fixture_init(&f);
    CHECK(pm_manager_push(f.manager, "A", NULL));
    CHECK(pm_page_set_animation(f.pages[1], PM_ANIM_MOVE_LEFT, 200, NULL));
    CHECK(pm_manager_push(f.manager, "B", NULL));
    tick(50);
    /* Public LVGL cancellation must release the navigation lock. */
    CHECK(lv_anim_delete(f.manager, NULL));
    CHECK(!pm_manager_is_busy(f.manager));
    CHECK(f.contexts[1].calls[DID_APPEAR] == 1);
    CHECK(pm_manager_pop(f.manager));
    CHECK(pm_manager_clear(f.manager));
    CHECK(pm_manager_get_depth(f.manager) == 0);
    tick(250); /* no callback should access cleared roots */
    CHECK(pm_manager_push(f.manager, "B", NULL));
    CHECK(pm_manager_delete(f.manager));
    tick(250); /* no callback should access the deleted manager */

    /* Warm the theme/draw caches, then compare LVGL's allocator accounting. */
    fixture_init(&f);
    CHECK(pm_manager_push(f.manager, "A", NULL));
    CHECK(pm_manager_delete(f.manager));
    tick(100);
    lv_mem_monitor(&before);
    for(i = 0; i < 100; i++) {
        int argument = (int)i;
        pm_stash_t stash = PM_STASH_MAKE(argument);
        fixture_init(&f);
        CHECK(pm_manager_push(f.manager, "A", NULL));
        CHECK(pm_manager_push(f.manager, "B", &stash));
        CHECK(pm_manager_push(f.manager, "C", &stash));
        CHECK(pm_manager_back_home(f.manager));
        CHECK(pm_manager_push(f.manager, "D", &stash));
        CHECK(pm_manager_delete(f.manager));
    }
    tick(100);
    lv_mem_monitor(&after);
    CHECK(before.free_size == after.free_size);
    CHECK(before.used_cnt == after.used_cnt);
    CHECK(lv_mem_test() == LV_RESULT_OK);
}

typedef struct { unsigned int created; unsigned int destroyed; } factory_context_t;
static bool factory_create(const char * class_name, const char * instance_name,
                            pm_page_descriptor_t * descriptor, void * user_data)
{
    factory_context_t * context = user_data;
    (void)instance_name;
    if(strcmp(class_name, "test") != 0) return false;
    descriptor->user_data = lv_malloc(32);
    CHECK(descriptor->user_data);
    context->created++;
    return true;
}
static void factory_destroy(void * page_data, void * user_data)
{
    factory_context_t * context = user_data;
    context->destroyed++;
    lv_free(page_data);
}
static void test_factory_and_validation(void)
{
    factory_context_t first = { 0 };
    factory_context_t second = { 0 };
    pm_page_factory_t factory = { factory_create, factory_destroy, &first };
    pm_manager_t * manager = pm_manager_create(NULL);
    pm_page_t * page;
    CHECK(manager);
    CHECK(!pm_manager_install(manager, "test", NULL));
    CHECK(pm_manager_set_factory(manager, &factory));
    CHECK(!pm_manager_install(manager, "missing", NULL));
    page = pm_manager_install(manager, "test", NULL);
    CHECK(page);
    CHECK(strcmp(pm_page_get_name(page), "test") == 0);
    CHECK(!pm_manager_install(manager, "test", NULL));
    CHECK(first.created == 1);
    CHECK(!pm_manager_set_animation(manager, PM_ANIM_GLOBAL, 100, NULL));
    CHECK(!pm_manager_set_animation(manager, (pm_anim_type_t)99, 100, NULL));
    CHECK(!pm_page_set_cache_mode(page, (pm_cache_mode_t)99));
    CHECK(!pm_page_set_animation(page, (pm_anim_type_t)-1, 10, NULL));
    CHECK(!pm_page_set_animation(page, PM_ANIM_OVER_LEFT, UINT32_MAX, NULL));
    CHECK(!pm_manager_set_drag(manager, true, 0));
    CHECK(!pm_manager_set_drag(manager, true, 101));
    CHECK(pm_manager_set_animation(manager, PM_ANIM_NONE, 0, NULL));
    CHECK(pm_manager_push(manager, "test", NULL));
    CHECK(!pm_manager_uninstall(manager, "test"));
    CHECK(pm_manager_clear(manager));
    factory.user_data = &second;
    CHECK(pm_manager_set_factory(manager, &factory));
    CHECK(pm_manager_install(manager, "test", "other"));
    CHECK(pm_manager_uninstall(manager, "test"));
    CHECK(first.destroyed == 1 && second.destroyed == 0);
    CHECK(pm_manager_delete(manager));
    CHECK(second.created == 1 && second.destroyed == 1);
    CHECK(!pm_manager_push(NULL, "A", NULL));
    CHECK(!pm_manager_delete(NULL));
    CHECK(!pm_page_stash_pop(NULL, NULL, 0));
}

static lv_indev_data_t pointer_data;
static void pointer_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    (void)indev;
    *data = pointer_data;
}
static void pointer_send(lv_indev_t * indev, int32_t x, int32_t y, bool pressed)
{
    pointer_data.point.x = x;
    pointer_data.point.y = y;
    pointer_data.state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    lv_tick_inc(10);
    lv_indev_read(indev);
    (void)lv_timer_handler();
}
static void test_drag(void)
{
    pm_anim_type_t type;
    lv_indev_t * indev = lv_indev_create();
    CHECK(indev);
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, display);
    lv_indev_set_read_cb(indev, pointer_read);
    lv_indev_set_mode(indev, LV_INDEV_MODE_EVENT);
    for(type = PM_ANIM_OVER_LEFT; type <= PM_ANIM_OVER_BOTTOM; type++) {
        fixture_t f;
        int32_t x = reversed(type) && !vertical(type) ? 300 : 20;
        int32_t y = reversed(type) && vertical(type) ? 220 : 100;
        int32_t delta = reversed(type) ? -1 : 1;
        unsigned int i;
        fixture_init(&f);
        CHECK(pm_manager_push(f.manager, "A", NULL));
        CHECK(pm_page_set_animation(f.pages[1], type, 100, NULL));
        CHECK(pm_manager_push(f.manager, "B", NULL));
        settle(f.manager);
        pointer_send(indev, x, y, true);
        CHECK(pm_manager_is_busy(f.manager));
        CHECK(!lv_obj_has_flag(pm_page_get_root(f.pages[0]), LV_OBJ_FLAG_HIDDEN));
        /* Short, stationary release returns to the current page. */
        if(vertical(type)) y += delta * 10; else x += delta * 10;
        pointer_send(indev, x, y, true);
        pointer_send(indev, x, y, true);
        pointer_send(indev, x, y, false);
        settle(f.manager);
        CHECK(pm_manager_get_depth(f.manager) == 2);
        CHECK(lv_obj_has_flag(pm_page_get_root(f.pages[0]), LV_OBJ_FLAG_HIDDEN));
        CHECK(f.contexts[0].calls[DID_APPEAR] == 1);
        CHECK(f.contexts[1].calls[WILL_DISAPPEAR] == 0);
        pointer_send(indev, x, y, true);
        for(i = 0; i < 5; i++) {
            if(vertical(type)) y += delta * 20; else x += delta * 35;
            pointer_send(indev, x, y, true);
        }
        /* Inertia makes the vertical 100px movement exceed the 120px threshold. */
        pointer_send(indev, x, y, false);
        settle(f.manager);
        CHECK(pm_manager_get_depth(f.manager) == 1);
        CHECK(pm_manager_get_current(f.manager) == f.pages[0]);
        CHECK(!pm_page_is_loaded(f.pages[1]));
        CHECK(pm_manager_delete(f.manager));
    }
    /* No drag when the previous view was intentionally not cached. */
    {
        fixture_t f;
        fixture_init(&f);
        CHECK(pm_page_set_cache_mode(f.pages[0], PM_CACHE_NONE));
        CHECK(pm_manager_push(f.manager, "A", NULL));
        CHECK(pm_page_set_animation(f.pages[1], PM_ANIM_OVER_LEFT, 100, NULL));
        CHECK(pm_manager_push(f.manager, "B", NULL));
        settle(f.manager);
        pointer_send(indev, 20, 100, true);
        CHECK(!pm_manager_is_busy(f.manager));
        pointer_send(indev, 250, 100, true);
        pointer_send(indev, 250, 100, false);
        CHECK(pm_manager_get_depth(f.manager) == 2);
        CHECK(pm_manager_pop(f.manager));
        settle(f.manager);
        CHECK(f.contexts[0].calls[LOAD] == 2);
        CHECK(pm_manager_delete(f.manager));
    }
    lv_indev_delete(indev);
}

static void test_example(void)
{
    pm_manager_t * manager = pm_example_create(NULL);
    pm_page_t * home;
    pm_page_t * details;
    CHECK(manager);
    settle(manager);
    home = pm_manager_get_current(manager);
    CHECK(lv_obj_send_event(lv_obj_get_child(pm_page_get_root(home), 1), LV_EVENT_CLICKED, NULL) == LV_RESULT_OK);
    settle(manager);
    details = pm_manager_get_current(manager);
    CHECK(strcmp(pm_page_get_name(details), "details") == 0);
    CHECK(strstr(lv_label_get_text(lv_obj_get_child(pm_page_get_root(details), 0)), "Visit 1"));
    /* The back button may delete itself after a synchronous transition; the
     * example uses an animation, so this event returns normally. */
    CHECK(lv_obj_send_event(lv_obj_get_child(pm_page_get_root(details), 1), LV_EVENT_CLICKED, NULL) == LV_RESULT_OK);
    settle(manager);
    CHECK(pm_manager_get_current(manager) == home);
    CHECK(pm_manager_delete(manager));
}

int main(void)
{
    lv_init();
    display = lv_display_create(320, 240);
    CHECK(display);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    test_lifecycle_and_stash();
    test_animations();
    test_cancel_and_memory();
    test_factory_and_validation();
    test_drag();
    test_example();
    lv_display_delete(display);
    lv_deinit();
    puts("PageManager integration tests passed (LVGL " LVGL_VERSION_INFO ")");
    return EXIT_SUCCESS;
}
