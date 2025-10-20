#include "page_manager_i.h"
#include "pm_anim.h"
#include <string.h>

bool pm_switch_anim_state_check(pm_manager_t* m)
{
    struct pm_manager* im = (struct pm_manager*)m;
    if (im->anim.IsSwitchReq || im->anim.IsBusy) return false;
    return true;
}

pm_page_t* pm_find_in_stack(pm_manager_t* m, const char* name)
{
    pm_page_vec_t tmp = ((struct pm_manager*)m)->stack; /* copy */
    while (kv_size(tmp)) {
        pm_page_t* base = kv_A(tmp, kv_size(tmp)-1);
        if (base && base->name && name && strcmp(name, base->name) == 0) return base;
        kv_size(tmp)--;
    }
    return 0;
}

bool pm_get_current_anim_attr(pm_manager_t* m, pm_load_anim_attr_t* out)
{
    return pm_anim_get_attr(((struct pm_manager*)m)->anim.Current.Type, out);
}

void pm_switch_anim_type_update(pm_manager_t* m, pm_page_t* base)
{
    struct pm_manager* im = (struct pm_manager*)m;
    if (base->priv.Anim.Attr.Type == PM_LOAD_ANIM_GLOBAL) {
        im->anim.Current = im->anim.Global;
    } else {
        if (base->priv.Anim.Attr.Type > PM__LOAD_ANIM_LAST) base->priv.Anim.Attr = im->anim.Global;
        im->anim.Current = base->priv.Anim.Attr;
    }
}

bool pm_switch_to(pm_manager_t* m, pm_page_t* new_page, bool is_enter, const pm_stash_t* stash)
{
    struct pm_manager* im = (struct pm_manager*)m;
    if (!new_page) return false;
    if (im->anim.IsSwitchReq) return false;
    im->anim.IsSwitchReq = true;

    if (stash) {
        void* buffer = 0;
        if (!new_page->priv.Stash.ptr) {
            buffer = lv_mem_alloc(stash->size);
        } else if (new_page->priv.Stash.size == stash->size) {
            buffer = new_page->priv.Stash.ptr;
        }
        if (buffer) {
            memcpy(buffer, stash->ptr, stash->size);
            new_page->priv.Stash.ptr = buffer;
            new_page->priv.Stash.size = stash->size;
        }
    }

    im->current = new_page;
    if (im->current->priv.IsCached) {
        im->current->priv.State = PM_PAGE_STATE_WILL_APPEAR;
    } else {
        im->current->priv.State = PM_PAGE_STATE_LOAD;
    }
    if (im->prev) im->prev->priv.Anim.IsEnter = false;
    im->current->priv.Anim.IsEnter = true;
    im->anim.IsEntering = is_enter;
    if (im->anim.IsEntering) pm_switch_anim_type_update(m, im->current);
    extern void pm_state_update(pm_manager_t*, pm_page_t*);
    pm_state_update(m, im->prev);
    pm_state_update(m, im->current);

    if (im->anim.IsEntering) {
        if (im->prev) lv_obj_move_foreground(im->prev->root);
        lv_obj_move_foreground(im->current->root);
    } else {
        lv_obj_move_foreground(im->current->root);
        if (im->prev) lv_obj_move_foreground(im->prev->root);
    }
    return true;
}


