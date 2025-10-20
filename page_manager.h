/* C Page Manager public API */
#ifndef PAGE_MANAGER_C_H_
#define PAGE_MANAGER_C_H_

#include "pm_factory.h"
#include "pm_types.h"


#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct pm_manager pm_manager_t;

    pm_manager_t *pm_manager_create(const pm_factory_t *factory /*nullable*/);
    void pm_manager_destroy(pm_manager_t *);

    bool pm_manager_install(pm_manager_t *, const char *class_name, const char *app_name /*nullable*/);
    bool pm_manager_uninstall(pm_manager_t *, const char *app_name);
    bool pm_manager_register(pm_manager_t *, pm_page_t *page, const char *name);
    bool pm_manager_unregister(pm_manager_t *, const char *name);

    bool pm_manager_replace(pm_manager_t *, const char *name, const pm_stash_t *stash /*nullable*/);
    bool pm_manager_push(pm_manager_t *, const char *name, const pm_stash_t *stash /*nullable*/);
    bool pm_manager_pop(pm_manager_t *);
    bool pm_manager_back_home(pm_manager_t *);
    const char *pm_manager_get_prev_name(const pm_manager_t *);

    void pm_manager_set_global_load_anim(pm_manager_t *, uint8_t anim, uint16_t time, lv_anim_path_cb_t path);
    void pm_manager_set_root_default_style(pm_manager_t *, lv_style_t *style);

#ifdef __cplusplus
}
#endif

#endif /* PAGE_MANAGER_C_H_ */
