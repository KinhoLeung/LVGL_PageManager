#include "page_manager.h"
#include "page_manager_i.h"
#include "pm_anim.h"
#include "pm_log.h"

/* Internal hooks provided by other modules */
void pm_switch_anim_create(pm_manager_t* m, pm_page_t* base);
void pm_root_enable_drag(pm_manager_t* m, lv_obj_t* root);
bool pm_get_is_over_anim(pm_manager_t* m, uint8_t animType);
bool pm_get_current_anim_attr(pm_manager_t* m, pm_load_anim_attr_t* out);

/* Access to manager internals */
struct pm_manager;
pm_page_t* pm_stack_top_after(pm_manager_t*);
pm_page_t* pm_stack_top(pm_manager_t*);
static pm_page_state_t pm_state_load(pm_manager_t* m, pm_page_t* base)
{
    if (base->root != 0) {
        /* root must be null before load */
    }
    lv_obj_t* root_obj = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(root_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(root_obj, base);
    extern lv_style_t* pm_get_root_default_style(struct pm_manager*);
    lv_style_t* style = pm_get_root_default_style((struct pm_manager*)m);
    if (style) {
        lv_obj_add_style(root_obj, style, LV_PART_MAIN);
    }
    base->root = root_obj;
    if (base->vtable && base->vtable->onViewLoad) base->vtable->onViewLoad(base);

    if (pm_get_is_over_anim(m, ((struct pm_manager*)m)->anim.Current.Type)) {
        pm_page_t* bottom = pm_stack_top_after((struct pm_manager*)m);
        if (bottom && bottom->priv.IsCached) {
            pm_load_anim_attr_t attr;
            if (pm_get_current_anim_attr(m, &attr)) {
                if (attr.dragDir != PM_ROOT_DRAG_DIR_NONE) pm_root_enable_drag(m, base->root);
            }
        }
    }

    if (base->vtable && base->vtable->onViewDidLoad) base->vtable->onViewDidLoad(base);

    if (base->priv.IsDisableAutoCache) {
        base->priv.IsCached = base->priv.ReqEnableCache;
    } else {
        base->priv.IsCached = true;
    }

    return PM_PAGE_STATE_WILL_APPEAR;
}

static pm_page_state_t pm_state_will_appear(pm_manager_t* m, pm_page_t* base)
{
    if (base->vtable && base->vtable->onViewWillAppear) base->vtable->onViewWillAppear(base);
    lv_obj_clear_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    pm_switch_anim_create(m, base);
    return PM_PAGE_STATE_DID_APPEAR;
}

static pm_page_state_t pm_state_did_appear(pm_manager_t* m, pm_page_t* base)
{
    (void)m;
    if (base->vtable && base->vtable->onViewDidAppear) base->vtable->onViewDidAppear(base);
    return PM_PAGE_STATE_ACTIVITY;
}

static pm_page_state_t pm_state_will_disappear(pm_manager_t* m, pm_page_t* base)
{
    if (base->vtable && base->vtable->onViewWillDisappear) base->vtable->onViewWillDisappear(base);
    pm_switch_anim_create(m, base);
    return PM_PAGE_STATE_DID_DISAPPEAR;
}

static pm_page_state_t pm_state_did_disappear(pm_manager_t* m, pm_page_t* base)
{
    (void)m;
    lv_obj_add_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    if (base->vtable && base->vtable->onViewDidDisappear) base->vtable->onViewDidDisappear(base);
    if (base->priv.IsCached) return PM_PAGE_STATE_WILL_APPEAR;
    return PM_PAGE_STATE_UNLOAD;
}

static pm_page_state_t pm_state_unload(pm_manager_t* m, pm_page_t* base)
{
    (void)m;
    if (!base->root) return PM_PAGE_STATE_IDLE;
    if (base->vtable && base->vtable->onViewUnload) base->vtable->onViewUnload(base);
    if (base->priv.Stash.ptr && base->priv.Stash.size) {
        lv_mem_free(base->priv.Stash.ptr);
        base->priv.Stash.ptr = 0;
        base->priv.Stash.size = 0;
    }
    lv_obj_del_async(base->root);
    base->root = 0;
    base->priv.IsCached = false;
    if (base->vtable && base->vtable->onViewDidUnload) base->vtable->onViewDidUnload(base);
    return PM_PAGE_STATE_IDLE;
}

/* Exposed entry to drive state machine */
void pm_state_update(pm_manager_t* m, pm_page_t* base)
{
    if (!m || !base) return;
    switch (base->priv.State) {
    case PM_PAGE_STATE_IDLE:
        break;
    case PM_PAGE_STATE_LOAD:
        base->priv.State = pm_state_load(m, base);
        pm_state_update(m, base);
        break;
    case PM_PAGE_STATE_WILL_APPEAR:
        base->priv.State = pm_state_will_appear(m, base);
        break;
    case PM_PAGE_STATE_DID_APPEAR:
        base->priv.State = pm_state_did_appear(m, base);
        break;
    case PM_PAGE_STATE_ACTIVITY:
        base->priv.State = PM_PAGE_STATE_WILL_DISAPPEAR;
        pm_state_update(m, base);
        break;
    case PM_PAGE_STATE_WILL_DISAPPEAR:
        base->priv.State = pm_state_will_disappear(m, base);
        break;
    case PM_PAGE_STATE_DID_DISAPPEAR:
        base->priv.State = pm_state_did_disappear(m, base);
        if (base->priv.State == PM_PAGE_STATE_UNLOAD) pm_state_update(m, base);
        break;
    case PM_PAGE_STATE_UNLOAD:
        base->priv.State = pm_state_unload(m, base);
        break;
    default:
        break;
    }
}


