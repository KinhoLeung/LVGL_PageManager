/* SPDX-License-Identifier: MIT */
#ifndef LVGL_PM_EXAMPLE_H
#define LVGL_PM_EXAMPLE_H
#include "PageManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Call after initializing LVGL and the display. Delete the returned manager
 * before deleting parent; no application-owned context needs to be freed. */
pm_manager_t * pm_example_create(lv_obj_t * parent);
#ifdef __cplusplus
}
#endif
#endif
