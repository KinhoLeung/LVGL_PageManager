/* C equivalent for PageBase */
#ifndef PAGE_BASE_C_H_
#define PAGE_BASE_C_H_

#include "pm_log.h"
#include "pm_types.h"


struct pm_manager; /* fwd */

typedef struct pm_page pm_page_t;

typedef struct
{
    void (*onCustomAttrConfig)(pm_page_t *);
    void (*onViewLoad)(pm_page_t *);
    void (*onViewDidLoad)(pm_page_t *);
    void (*onViewWillAppear)(pm_page_t *);
    void (*onViewDidAppear)(pm_page_t *);
    void (*onViewWillDisappear)(pm_page_t *);
    void (*onViewDidDisappear)(pm_page_t *);
    void (*onViewUnload)(pm_page_t *);
    void (*onViewDidUnload)(pm_page_t *);
} pm_page_vtable_t;

struct pm_page_priv_anim
{
    bool IsEnter;
    bool IsBusy;
    pm_anim_attr_t Attr;
};

typedef struct pm_page_priv
{
    bool ReqEnableCache;
    bool ReqDisableAutoCache;
    bool IsDisableAutoCache;
    bool IsCached;
    pm_stash_t Stash;
    pm_page_state_t State;
    struct pm_page_priv_anim Anim;
} pm_page_priv_t;

struct pm_page
{
    lv_obj_t *root;
    struct pm_manager *manager;
    const char *name;
    uint16_t id;
    void *user_data;

    pm_page_priv_t priv;
    const pm_page_vtable_t *vtable; /* callbacks */
};

#ifdef __cplusplus
extern "C"
{
#endif

    pm_page_t *pm_page_create(const pm_page_vtable_t *vtable);
    void pm_page_destroy(pm_page_t *page);

    void pm_page_set_custom_cache_enable(pm_page_t *, bool en);
    void pm_page_set_custom_auto_cache_enable(pm_page_t *, bool en);
    void pm_page_set_custom_load_anim(pm_page_t *, uint8_t type, uint16_t time, lv_anim_path_cb_t path);
    bool pm_page_stash_pop(pm_page_t *, void *out, uint32_t size);

#define PM_PAGE_STASH_MAKE(data) ((pm_stash_t){&(data), (uint32_t)sizeof(data)})

#ifdef __cplusplus
}
#endif

#endif /* PAGE_BASE_C_H_ */
