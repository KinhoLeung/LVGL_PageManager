/* Factory interface for creating pages by class name */
#ifndef PM_FACTORY_H_
#define PM_FACTORY_H_

#include "page_base.h"
#include "pm_types.h"


typedef pm_page_t *(*pm_factory_create_cb)(const char *class_name, void *user);
typedef void (*pm_factory_destroy_cb)(pm_page_t *page, void *user);

typedef struct
{
    pm_factory_create_cb create;
    pm_factory_destroy_cb destroy; /* optional, NULL => default destroy */
    void *user;                    /* opaque user context */
} pm_factory_t;

#endif /* PM_FACTORY_H_ */
