[English](README.md) | [中文](README_zh.md)

# LVGL_PageManager

A standalone **C99 / LVGL 9.x** page manager derived from the X-TRACK PageManager. It uses callback tables for page lifecycles and linked lists for the page registry and navigation stack. The component depends on LVGL, with no STL, Arduino, application framework, or additional task required.

Features include page registration, an optional factory, Push / Pop / Replace / BackHome navigation, caching, copied navigation parameters, transitions, and drag-to-go-back. Only LVGL 9.x is supported; other major versions are rejected at compile time.

## Files and integration

| File | Purpose |
| --- | --- |
| `PageManager.h` | Manager, registration, and navigation API |
| `PageBase.h` | Page callbacks, attributes, cache policies, and stash API |
| `PageFactory.h` | Optional page factory API |
| `PageManager.c` | Registry, navigation stack, and lifecycles |
| `PageBase.c` | Page attributes and stash handling |
| `PM_Anim.c` | Transition animations |
| `PM_Drag.c` | Drag back and snap-back animations |
| `PM_Private.h` | Internal implementation; do not access it from application code |
| `examples/` | An interactive home/details example |
| `tests/` | Headless integration tests using real LVGL |

For manual integration, compile the four `.c` files listed above, add this directory to the include path, and link your existing LVGL 9 library. The default header is `"lvgl.h"`; define `PM_LVGL_INCLUDE="lvgl/lvgl.h"` for a different include layout. Your application supplies `lv_conf.h`, the display driver, and the input driver.

For CMake:

```cmake
add_subdirectory(path/to/lvgl)             # Omit if your project already provides LVGL
add_subdirectory(path/to/LVGL_PageManager)
target_link_libraries(your_app PRIVATE lvgl_page_manager::lvgl_page_manager)
```

The component recognizes `lvgl::lvgl`, `lvgl`, and `idf::lvgl` targets. If your LVGL target has another name, link it to `lvgl_page_manager` yourself.

## Minimal example

```c
#include "PageManager.h"

static void home_load(pm_page_t * page)
{
    lv_obj_t * label = lv_label_create(pm_page_get_root(page));
    lv_label_set_text(label, "Home");
    lv_obj_center(label);
}

static void details_load(pm_page_t * page)
{
    lv_obj_t * label = lv_label_create(pm_page_get_root(page));
    lv_label_set_text(label, "Details");
    lv_obj_center(label);
}

static pm_manager_t * manager;

void app_ui_init(void)
{
    /* Initialize LVGL and the display before calling this function. */
    pm_page_descriptor_t home = {
        .callbacks = { .on_view_load = home_load }
    };
    pm_page_descriptor_t details = {
        .callbacks = { .on_view_load = details_load }
    };

    manager = pm_manager_create(NULL); /* NULL selects the active screen. */
    if(!manager) return;
    if(!pm_manager_register(manager, "home", &home) ||
       !pm_manager_register(manager, "details", &details) ||
       !pm_manager_push(manager, "home", NULL)) {
        pm_manager_delete(manager);
        manager = NULL;
    }
}

bool app_open_details(void)
{
    return pm_manager_push(manager, "details", NULL);
}

bool app_back(void)
{
    return pm_manager_pop(manager);
}

void app_ui_deinit(void)
{
    if(manager && pm_manager_delete(manager)) manager = NULL;
}
```

For an interactive example, compile [`examples/page_manager_example.c`](examples/page_manager_example.c) and call `pm_example_create(NULL)` after initializing the display. Keep the returned manager and delete it during UI teardown.

## Lifecycle callbacks

Every callback has the signature `void callback(pm_page_t * page)`. Unused callbacks may be `NULL`.

```text
Registration: on_custom_attr_config

First appearance / reloading:
on_view_load -> on_view_did_load -> on_view_will_appear
             -> [animation] -> on_view_did_appear

Disappearance:
on_view_will_disappear -> [animation] -> on_view_did_disappear
    -> keep the root for a future appearance
    or on_view_unload -> delete root -> on_view_did_unload

Returning from cache:
on_view_will_appear -> [animation] -> on_view_did_appear
```

A transition runs the outgoing page's `will_disappear`, loads the incoming page if needed, runs its `will_appear`, plays the animation, completes the outgoing disappearance and optional unloading, then calls the incoming `did_appear`. The root exists before `on_view_load` and is `NULL` in `on_view_did_unload`.

Use `pm_page_get_user_data()` to access your page context. Page callbacks can configure page cache policy, animation, and ID. Pages must release their own timers, subscriptions, and business resources in the appropriate disappearance or unloading callbacks. The manager deletes each root and its child objects.

**Run all API calls in the LVGL thread or under your application's LVGL lock.** Navigation, registration, removal, and manager configuration requests fail during animations, dragging, or lifecycle callbacks. The manager remains busy inside `did_appear` as well. To navigate from a lifecycle callback, defer the request with an application timer or `lv_async_call()` and manage the lifetime and cancellation of that deferred work.

## Navigation and ownership

| API | Behavior |
| --- | --- |
| `pm_manager_register` | Copies the name and descriptor; returns a manager-owned page handle |
| `pm_manager_push` | Pushes a registered page that is not already in the stack |
| `pm_manager_pop` | Pops the top page and returns to the next one, preserving the last page |
| `pm_manager_replace` | Replaces the top page, always unloads the outgoing page, and preserves stack depth |
| `pm_manager_back_home` | Returns to the bottom page, unloads removed pages, and animates the outgoing top page |
| `pm_manager_clear` | Clears the stack and all cached roots, keeping registrations |
| `pm_manager_unregister` / `uninstall` | Rejects pages still in the stack; unloads their cache and releases the handle |
| `pm_manager_delete` | Stops animations, ends lifecycles, and releases the manager |

Names must be nonempty and unique. A page cannot appear twice in the stack. Invalid requests return `false` or `NULL` without changing the stack. `pop` and `back_home` fail for an empty stack or a single home page; `replace` requires a current page.

`clear` and `delete` can stop an animation or an active drag. An accepted transition is completed before its pages are cleaned up. These operations cannot reenter the manager from lifecycle callbacks or LVGL object events being dispatched by the component. Page handles become invalid after removal or manager deletion and must not be freed by the application.

Each manager owns a container, with one root per loaded page. Containers and roots fill their parent by default. Transition distances use the container's actual content size, allowing custom parent containers and multiple managers. Page root position, size, visibility, and overall opacity are reserved for the manager; put your application layout inside the root.

`pm_manager_set_root_style()` borrows a style pointer and applies it to subsequently created page roots. Keep the style alive until those roots have been deleted.

**Delete the manager before deleting its parent screen/display or calling `lv_deinit()`.** Do not directly delete the manager's container or page roots. For direct registrations, the application owns `user_data`; descriptors, callback tables, and names are copied and may be local variables.

`pm_manager_get_previous()` returns the page below the stack top. `pm_manager_get_previous_name()` returns the source of the most recent or current transition, or `NULL` when there is no source.

## Caching

```c
pm_page_set_cache_mode(page, PM_CACHE_AUTO); /* Default */
pm_page_set_cache_mode(page, PM_CACHE_KEEP);
pm_page_set_cache_mode(page, PM_CACHE_NONE);
```

| Policy | Covered by a new page | After Pop |
| --- | --- | --- |
| `PM_CACHE_AUTO` | Keep the root | Unload the root |
| `PM_CACHE_KEEP` | Keep the root | Keep the root for reuse by a later Push |
| `PM_CACHE_NONE` | Unload the root | Unload the root |

A page with `PM_CACHE_NONE` can remain in the stack and is loaded again when returning to it. Replace, BackHome, Clear, and manager deletion force unloading of removed pages, including those configured as `KEEP`. Changing cache policy affects the next disappearance; it does not immediately unload an already hidden cached page.

## Navigation parameters: stash

```c
typedef struct { uint32_t item_id; } details_args_t;
details_args_t args = { 123 };
pm_stash_t stash = PM_STASH_MAKE(args);
bool opened = pm_manager_push(manager, "details", &stash);

/* In the destination page's on_view_will_appear callback: */
details_args_t received;
if(pm_page_stash_pop(page, &received, sizeof(received))) {
    /* Use received.item_id. */
}
```

Parameters are allocated and copied before the navigation stack changes. The caller's data may be released after the function returns. New data replaces any unconsumed stash, and its size may change. Passing `NULL` to Push or Replace clears old parameters. Pop and BackHome do not provide new parameters.

`pm_page_stash_pop()` requires an exact size match and releases the stash after a successful copy; failed reads do not consume it. `pm_page_stash_peek()` exposes the data and size until the next stash update, consumption, clearing, or page unloading. Unloading releases unconsumed parameters. Use self-contained values: pointers inside a copied structure are not recursively copied.

## Transitions and drag back

```c
pm_manager_set_animation(manager, PM_ANIM_OVER_LEFT, 300, lv_anim_path_ease_out);
pm_page_set_animation(page, PM_ANIM_MOVE_TOP, 200, lv_anim_path_linear);
pm_page_set_animation(page, PM_ANIM_GLOBAL, 0, NULL); /* Inherit manager settings */
pm_manager_set_drag(manager, true, 50);               /* Default drag settings */
```

Available transitions are `OVER_LEFT/RIGHT/TOP/BOTTOM`, `MOVE_LEFT/RIGHT/TOP/BOTTOM`, `FADE_ON`, and `NONE`. The direction describes the incoming page's movement: `OVER_LEFT` enters from the right and supports dragging right to go back. A `NULL` path selects `lv_anim_path_ease_out`. Durations are milliseconds in `0..INT32_MAX`; the manager's global animation cannot be `GLOBAL`.

Push and Replace save the entry animation configuration. Pop reverses that saved configuration, so later setting changes do not alter an existing page's return direction. `NONE` or zero duration completes synchronously without creating a zero-duration LVGL animation. Fading changes the opacity of the entire page subtree.

Drag back is enabled by default for `OVER_*` transitions when the page below has a loaded root. Dragging the root background temporarily reveals that page. Release combines displacement and inertia: reaching the threshold triggers Pop; otherwise the page snaps back. Temporarily revealing a page does not invoke its appearance callbacks. Child controls keep their own gestures, and their bubbled events do not initiate drag back. `PRESS_LOST` cancels the drag. Thresholds range from 1 to 100 percent.

## Optional factory

```c
#include <string.h>

static bool create_page(const char * class_name, const char * instance_name,
                        pm_page_descriptor_t * result, void * factory_data)
{
    (void)instance_name;
    (void)factory_data;
    if(strcmp(class_name, "HomePage") != 0) return false;
    *result = (pm_page_descriptor_t) {
        .callbacks = { .on_view_load = home_load }
    };
    return true;
}

pm_page_factory_t factory = { .create = create_page };
pm_manager_set_factory(manager, &factory);
pm_page_t * home = pm_manager_install(manager, "HomePage", "home");
```

The factory fills a descriptor rather than allocating a page handle. A `NULL` instance name uses the class name. If the factory allocates a page context, provide `destroy(page_user_data, factory_user_data)` to release it when the registration or manager is deleted. A successful factory result followed by a registration allocation failure also invokes `destroy`. A factory returning failure must clean up its own temporary resources.

Each installed page remembers the destroy callback and factory context used during installation. Changing the manager's factory does not change existing pages' cleanup behavior. Keep each factory context valid until its installed registrations have been released.

## Validation

The tests use real LVGL display objects, animation timers, and input devices. The component and example have passed builds and integration tests with **LVGL 9.0.0, 9.2.2, and 9.5.0**, using C99 and `-Wall -Wextra -Werror -pedantic` for the component and test sources.

Run from this repository's root:

```sh
cmake -S tests -B build -DLVGL_DIR=/absolute/path/to/lvgl-9.x
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Windows with MinGW, add `-G "MinGW Makefiles"` to the configuration command. `tests/lv_conf.h` is for standalone tests; applications should supply their own configuration.

Coverage includes navigation and lifecycle order, all three cache policies, stash copies and size changes, allocation failure without stack changes, every transition type, saved return animations, busy and reentrancy handling, animation cancellation, clear/delete during transitions, factory cleanup, drag back in all four directions, memory accounting across repeated creation/deletion, and the example's button navigation.

## License

MIT, retaining the original X-TRACK PageManager attribution. See [LICENSE](LICENSE).

LVGL API references: [animations](https://docs.lvgl.io/9.2/overview/animations.html) and [objects](https://docs.lvgl.io/9.2/API/core/lv_obj.html).
