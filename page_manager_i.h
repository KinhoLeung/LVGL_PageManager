/* Internal manager definition shared across modules */
#ifndef PAGE_MANAGER_I_H_
#define PAGE_MANAGER_I_H_

#include "kvec.h"
#include "page_manager.h"


typedef kvec_t(pm_page_t *) pm_page_vec_t;

struct pm_manager_anim_state
{
    bool IsSwitchReq;
    bool IsBusy;
    bool IsEntering;
    pm_anim_attr_t Current;
    pm_anim_attr_t Global;
};

struct pm_manager
{
    const pm_factory_t *factory;
    pm_page_vec_t pool;
    pm_page_vec_t stack;
    pm_page_t *prev;
    pm_page_t *current;
    struct pm_manager_anim_state anim;
    lv_style_t *root_default_style;
};

/* Internal helpers exposed across C modules */
pm_page_t *pm_find_in_pool(pm_manager_t *m, const char *name);
pm_page_t *pm_find_in_stack(pm_manager_t *m, const char *name);
pm_page_t *pm_stack_top(pm_manager_t *m);
pm_page_t *pm_stack_top_after(pm_manager_t *m);
void pm_stack_push(pm_manager_t *m, pm_page_t *p);
void pm_stack_pop(pm_manager_t *m);
void pm_stack_clear_to_bottom(pm_manager_t *m);
void pm_set_prev(pm_manager_t *m, pm_page_t *p);
lv_style_t *pm_get_root_default_style(pm_manager_t *m);

bool pm_switch_anim_state_check(pm_manager_t *m);
bool pm_get_current_anim_attr(pm_manager_t *m, pm_load_anim_attr_t *out);
bool pm_get_is_over_anim(pm_manager_t *m, uint8_t animType);
void pm_switch_anim_type_update(pm_manager_t *m, pm_page_t *base);
bool pm_switch_to(pm_manager_t *m, pm_page_t *new_page, bool is_enter, const pm_stash_t *stash);
void pm_state_update(pm_manager_t *m, pm_page_t *base);

#endif /* PAGE_MANAGER_I_H_ */
