#include "page_manager.h"
#include "pm_log.h"
#include "page_manager_i.h"
#include <string.h>

pm_page_t* pm_find_in_pool(pm_manager_t* m, const char* name)
{
    for (size_t i = 0; i < kv_size(m->pool); ++i) {
        pm_page_t* p = kv_A(m->pool, i);
        if (p && p->name && name && strcmp(p->name, name) == 0) return p;
    }
    return 0;
}

pm_page_t* pm_stack_top(pm_manager_t* m)
{
    if (kv_size(m->stack) == 0) return 0;
    return kv_A(m->stack, kv_size(m->stack) - 1);
}

pm_page_t* pm_stack_top_after(pm_manager_t* m)
{
    size_t n = kv_size(m->stack);
    if (n < 2) return 0;
    return kv_A(m->stack, n - 2);
}

pm_manager_t* pm_manager_create(const pm_factory_t* factory)
{
    pm_manager_t* m = (pm_manager_t*)lv_mem_alloc(sizeof(pm_manager_t));
    if (!m) return 0;
    memset(m, 0, sizeof(*m));
    m->factory = factory;
    kv_init(m->pool);
    kv_init(m->stack);
    m->anim.Global.Type = PM_LOAD_ANIM_OVER_LEFT;
    m->anim.Global.Time = PM_PAGE_ANIM_TIME_DEFAULT;
    m->anim.Global.Path = PM_PAGE_ANIM_PATH_DEFAULT;
    return m;
}

void pm_manager_destroy(pm_manager_t* m)
{
    if (!m) return;

    /* Clear stack without animations */
    while (kv_size(m->stack)) {
        pm_page_t* top = pm_stack_top(m);
        kv_pop(m->stack);
        if (top) {
            top->priv.IsCached = false;
            top->priv.State = PM_PAGE_STATE_UNLOAD;
            pm_state_update(m, top);
        }
    }

    /* Free pool entries if factory destroy provided */
    for (size_t i = 0; i < kv_size(m->pool); ++i) {
        pm_page_t* p = kv_A(m->pool, i);
        if (!p) continue;
        if (p->root) {
            p->priv.IsCached = false;
            p->priv.State = PM_PAGE_STATE_UNLOAD;
            pm_state_update(m, p);
        }
        if (m->factory && m->factory->destroy) m->factory->destroy(p, m->factory->user);
        else pm_page_destroy(p);
    }

    kv_destroy(m->pool);
    kv_destroy(m->stack);
    lv_mem_free(m);
}

bool pm_manager_register(pm_manager_t* m, pm_page_t* page, const char* name)
{
    if (!m || !page || !name) return false;
    if (pm_find_in_pool(m, name)) return false;
    page->manager = m;
    page->name = name;
    kv_push(pm_page_t*, m->pool, page);
    return true;
}

bool pm_manager_unregister(pm_manager_t* m, const char* name)
{
    if (!m || !name) return false;
    /* ensure not in stack */
    for (size_t i = 0; i < kv_size(m->stack); ++i) {
        pm_page_t* p = kv_A(m->stack, i);
        if (p && p->name && strcmp(p->name, name) == 0) return false;
    }
    for (size_t i = 0; i < kv_size(m->pool); ++i) {
        pm_page_t* p = kv_A(m->pool, i);
        if (p && p->name && strcmp(p->name, name) == 0) {
            /* erase by swap-pop */
            kv_A(m->pool, i) = kv_A(m->pool, kv_size(m->pool) - 1);
            kv_size(m->pool)--;
            return true;
        }
    }
    return false;
}

bool pm_manager_install(pm_manager_t* m, const char* class_name, const char* app_name)
{
    if (!m) return false;
    if (!m->factory || !m->factory->create) return false;
    const char* name = app_name ? app_name : class_name;
    if (pm_find_in_pool(m, name)) return false;
    pm_page_t* page = m->factory->create(class_name, m->factory->user);
    if (!page) return false;
    page->root = 0;
    page->id = 0;
    page->manager = 0;
    page->user_data = 0;
    memset(&page->priv, 0, sizeof(page->priv));
    bool ok = pm_manager_register(m, page, name);
    if (ok && page->vtable && page->vtable->onCustomAttrConfig) page->vtable->onCustomAttrConfig(page);
    if (!ok) {
        if (m->factory && m->factory->destroy) m->factory->destroy(page, m->factory->user);
        else pm_page_destroy(page);
    }
    return ok;
}

bool pm_manager_uninstall(pm_manager_t* m, const char* app_name)
{
    if (!m || !app_name) return false;
    pm_page_t* page = pm_find_in_pool(m, app_name);
    if (!page) return false;
    if (!pm_manager_unregister(m, app_name)) return false;

    if (page->priv.IsCached || page->root) {
        /* caller must ensure not active; lifecycle handled in state module */
        page->priv.IsCached = false;
        page->priv.State = PM_PAGE_STATE_UNLOAD;
        pm_state_update(m, page);
    }
    if (m->factory && m->factory->destroy) m->factory->destroy(page, m->factory->user);
    else pm_page_destroy(page);
    return true;
}

const char* pm_manager_get_prev_name(const pm_manager_t* m)
{
    static const char* empty = "EMPTY_PAGE";
    return (m && m->prev && m->prev->name) ? m->prev->name : empty;
}

void pm_manager_set_global_load_anim(pm_manager_t* m, uint8_t anim, uint16_t time, lv_anim_path_cb_t path)
{
    if (!m) return;
    if (anim > PM__LOAD_ANIM_LAST) anim = PM_LOAD_ANIM_NONE;
    m->anim.Global.Type = anim;
    m->anim.Global.Time = time;
    m->anim.Global.Path = path;
}

void pm_manager_set_root_default_style(pm_manager_t* m, lv_style_t* style)
{
    if (!m) return;
    m->root_default_style = style;
}

/* stack helpers */
void pm_stack_push(pm_manager_t* m, pm_page_t* p) { kv_push(pm_page_t*, m->stack, p); }
void pm_stack_pop(pm_manager_t* m) { if (kv_size(m->stack)) kv_size(m->stack)--; }
void pm_stack_clear_to_bottom(pm_manager_t* m)
{
    while (kv_size(m->stack) > 1) { kv_size(m->stack)--; }
}
void pm_set_prev(pm_manager_t* m, pm_page_t* p) { m->prev = p; }
lv_style_t* pm_get_root_default_style(pm_manager_t* m) { return m->root_default_style; }



