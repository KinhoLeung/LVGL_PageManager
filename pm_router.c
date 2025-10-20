#include "page_manager.h"
#include "page_manager_i.h"
#include "pm_anim.h"
#include "pm_log.h"

/* Forward declarations of internal helpers implemented in other modules */
static bool pm_switch_anim_state_check(pm_manager_t* m);
bool pm_switch_to(pm_manager_t* m, pm_page_t* new_page, bool is_enter, const pm_stash_t* stash);
pm_page_t* pm_find_in_pool(pm_manager_t* m, const char* name);
pm_page_t* pm_find_in_stack(pm_manager_t* m, const char* name);
pm_page_t* pm_stack_top(pm_manager_t* m);

/* Local access to pm_manager internals */
struct pm_manager;
struct pm_manager* pm_get_manager_handle(pm_manager_t*);

bool pm_manager_replace(pm_manager_t* m, const char* name, const pm_stash_t* stash)
{
    if (!m || !name) return false;
    if (!pm_switch_anim_state_check(m)) return false;
    if (pm_find_in_stack(m, name)) return false;
    pm_page_t* base = pm_find_in_pool(m, name);
    if (!base) return false;
    pm_page_t* top = pm_stack_top(m);
    if (!top) return false;
    top->priv.IsCached = false;
    base->priv.IsDisableAutoCache = base->priv.ReqDisableAutoCache;
    /* pop current */
    pm_stack_pop(m);
    pm_stack_push(m, base);
    return pm_switch_to(m, base, true, stash);
}

bool pm_manager_push(pm_manager_t* m, const char* name, const pm_stash_t* stash)
{
    if (!m || !name) return false;
    if (!pm_switch_anim_state_check(m)) return false;
    if (pm_find_in_stack(m, name)) return false;
    pm_page_t* base = pm_find_in_pool(m, name);
    if (!base) return false;
    base->priv.IsDisableAutoCache = base->priv.ReqDisableAutoCache;
    pm_stack_push(m, base);
    return pm_switch_to(m, base, true, stash);
}

bool pm_manager_pop(pm_manager_t* m)
{
    if (!m) return false;
    if (!pm_switch_anim_state_check(m)) return false;
    pm_page_t* top = pm_stack_top(m);
    if (!top) return false;
    if (!top->priv.IsDisableAutoCache) top->priv.IsCached = false;
    pm_stack_pop(m);
    top = pm_stack_top(m);
    return pm_switch_to(m, top, false, 0);
}

bool pm_manager_back_home(pm_manager_t* m)
{
    if (!m) return false;
    if (!pm_switch_anim_state_check(m)) return false;
    pm_stack_clear_to_bottom(m);
    pm_set_prev(m, 0);
    pm_page_t* home = pm_stack_top(m);
    return pm_switch_to(m, home, false, 0);
}


