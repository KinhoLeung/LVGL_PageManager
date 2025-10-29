[中文](README.zh.md) | [English](README.en.md)

# LVGL PageManager (C)

LVGL_PageManager is a pure C implementation of the PageManager pattern for LVGL v9. It keeps the familiar page lifecycle from the original C++ module while fitting straight into C-based firmware projects.

## Features
- Stack-based navigation: install, register, push, pop, replace, and jump home with `pm_manager_*` APIs.
- Full lifecycle callbacks (`onViewLoad` → `onViewDidUnload`) implemented through lightweight vtables.
- Built-in page caching controls plus a stash buffer for passing small blobs of state between pages.
- Configurable transition animations with global defaults, per-page overrides, and optional edge-drag gestures.
- Factory interface for dynamic page construction without hard-wiring constructors.
- Header-only logging shim that lets you plug in your own logging backend or stay silent by default.

## Getting Started
1. Copy the sources (`page_manager*.c/h`, `page_base*.c/h`, `pm_*`) into your LVGL application and compile them with your firmware.
2. Include `page_manager.h` wherever you need to drive navigation.
3. Provide an LVGL v9 include path (`lvgl.h` or `lvgl/lvgl.h`). Minimal stubs are provided for linting, but real builds must link against LVGL.

### Creating a Page
Implement the callbacks you need and create a page instance via `pm_page_create`.

```c
static void home_on_view_load(pm_page_t *page) {
    page->root = lv_obj_create(lv_scr_act());
    // build your layout here
}

static const pm_page_vtable_t HOME_VTABLE = {
    .onViewLoad = home_on_view_load,
    .onViewDidLoad = NULL,
    .onViewWillAppear = NULL,
    .onViewDidAppear = NULL,
    .onViewWillDisappear = NULL,
    .onViewDidDisappear = NULL,
    .onViewUnload = NULL,
    .onViewDidUnload = NULL,
};

pm_page_t *home_page_create(void) {
    return pm_page_create(&HOME_VTABLE);
}
```

Call `pm_page_set_custom_cache_enable`, `pm_page_set_custom_auto_cache_enable`, or `pm_page_set_custom_load_anim` from `onCustomAttrConfig` when you need per-page tweaks.

### Plugging in a Factory
The manager can request pages by class name at runtime. Provide a factory callback to keep construction logic in one place.

```c
static pm_page_t *factory_create(const char *class_name, void *user_ctx) {
    if (strcmp(class_name, "HomePage") == 0) return home_page_create();
    if (strcmp(class_name, "SettingsPage") == 0) return settings_page_create();
    return NULL;
}

static const pm_factory_t PAGE_FACTORY = {
    .create = factory_create,
    .destroy = NULL,   // defaults to pm_page_destroy
    .user = NULL,
};
```

### Driving Navigation
- `pm_manager_install` and `pm_manager_uninstall` dynamically add or remove page classes.
- `pm_manager_register` lets you register pre-built pages.
- `pm_manager_push`, `pm_manager_pop`, `pm_manager_replace`, and `pm_manager_back_home` navigate through the stack.
- `pm_manager_set_global_load_anim` and `pm_page_set_custom_load_anim` control transitions.
- `pm_page_stash_pop` retrieves data that was pushed alongside `pm_manager_push`/`pm_manager_replace` calls.

### Passing State with Stash
Provide a `pm_stash_t` when pushing or replacing a page to copy small structs (settings, IDs, etc.). Inside the target page, call `pm_page_stash_pop(page, &buffer, sizeof(buffer))` once to take ownership.

### Animation and Drag Support
Transitions default to `PM_LOAD_ANIM_OVER_LEFT`. Choose from overlay, move, fade, or disable animations entirely. Overlay animations automatically enable edge-drag interactions so users can cancel a transition mid-flight.

### Logging
`pm_log.h` defines `PM_LOG_INFO/WARN/ERROR` as no-ops. Define these macros before including the headers to forward logs to your existing system.

## Source Layout
- `page_manager*.c/h`, `pm_router*.c`, `pm_state.c` orchestrate the lifecycle and navigation stack.
- `pm_anim*.c/h` handles transition setup, easing, and drag gestures.
- `page_base*.c/h` defines the page object, vtable, and stash helpers.
- `pm_factory.h` describes the dynamic construction interface.
- `kvec.h` supplies the minimal dynamic array used for the pool and stack.

## Demo
Check `examples/demo.c` for a runnable starter that wires up two pages, demonstrates stash payloads, and shows the factory pattern in practice.

## Roadmap Ideas
- Optional helpers for registering C structs directly as pages.
- Example projects that mix the manager with LVGL demos.
- Additional animation curves beyond the LVGL defaults.

## License
The project currently ships without a declared license. Add one before publishing to clarify how others may use the code.
