#include "page_base.h"
#include <string.h>
#include <stddef.h>
#include <stdint.h>

pm_page_t* pm_page_create(const pm_page_vtable_t* vtable)
{
    pm_page_t* p = (pm_page_t*)lv_mem_alloc(sizeof(pm_page_t));
    if (!p) return (pm_page_t*)0;
    memset(p, 0, sizeof(*p));
    p->vtable = vtable;
    p->priv.Anim.Attr.Type = PM_LOAD_ANIM_GLOBAL;
    p->priv.Anim.Attr.Time = PM_PAGE_ANIM_TIME_DEFAULT;
    p->priv.Anim.Attr.Path = PM_PAGE_ANIM_PATH_DEFAULT;
    return p;
}

void pm_page_destroy(pm_page_t* page)
{
    if (!page) return;
    lv_mem_free(page);
}

void pm_page_set_custom_cache_enable(pm_page_t* p, bool en)
{
    if (!p) return;
    PM_LOG_INFO("Page(%s) pm_page_set_custom_cache_enable = %d", p->name ? p->name : "?", en);
    pm_page_set_custom_auto_cache_enable(p, false);
    p->priv.ReqEnableCache = en;
}

void pm_page_set_custom_auto_cache_enable(pm_page_t* p, bool en)
{
    if (!p) return;
    PM_LOG_INFO("Page(%s) pm_page_set_custom_auto_cache_enable = %d", p->name ? p->name : "?", en);
    p->priv.ReqDisableAutoCache = !en;
}

void pm_page_set_custom_load_anim(pm_page_t* p, uint8_t type, uint16_t time, lv_anim_path_cb_t path)
{
    if (!p) return;
    p->priv.Anim.Attr.Type = type;
    p->priv.Anim.Attr.Time = time;
    p->priv.Anim.Attr.Path = path;
}

bool pm_page_stash_pop(pm_page_t* p, void* out, uint32_t size)
{
    if (!p) return false;
    if (p->priv.Stash.ptr == 0) {
        PM_LOG_WARN("No Stash found");
        return false;
    }
    if (p->priv.Stash.size != size) {
        PM_LOG_WARN("Stash[%p](%u) does not match the size(%u)", p->priv.Stash.ptr, (unsigned)p->priv.Stash.size, (unsigned)size);
        return false;
    }
    memcpy(out, p->priv.Stash.ptr, p->priv.Stash.size);
    lv_mem_free(p->priv.Stash.ptr);
    p->priv.Stash.ptr = 0;
    p->priv.Stash.size = 0;
    return true;
}


