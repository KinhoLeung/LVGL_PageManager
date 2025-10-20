/* Animation helpers API */
#ifndef PM_ANIM_H_
#define PM_ANIM_H_

#include "page_manager.h"

bool pm_anim_get_attr(uint8_t anim, pm_load_anim_attr_t *attr);
void pm_anim_default_init(pm_manager_t *m, lv_anim_t *a);
void pm_switch_anim_create(pm_manager_t *m, pm_page_t *base);
bool pm_get_is_over_anim(pm_manager_t *m, uint8_t anim);

#endif /* PM_ANIM_H_ */
