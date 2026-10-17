/* SPDX-License-Identifier: MIT */
#ifndef LVGL_PM_PAGE_FACTORY_H
#define LVGL_PM_PAGE_FACTORY_H

#include "PageBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Optional factory. Fill a descriptor instead of constructing a C++ class.
 * On success, destroy owns descriptor.user_data even if registration fails.
 * Each installed page remembers its own destroy callback and factory context.
 */
typedef struct {
    bool (*create)(const char * class_name, const char * instance_name,
                   pm_page_descriptor_t * descriptor, void * factory_user_data);
    void (*destroy)(void * page_user_data, void * factory_user_data);
    void * user_data;
} pm_page_factory_t;

#ifdef __cplusplus
}
#endif
#endif
