/*
 * Common types for C PageManager (LVGL v9)
 */
#ifndef PM_TYPES_H_
#define PM_TYPES_H_

#include <stdbool.h>
#include <stdint.h>


#if defined(__has_include)
#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#else
/* Minimal fallbacks for linting when LVGL headers are unavailable */
#include <stddef.h>
/* Basic LVGL type stubs */
typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_style_t lv_style_t;
typedef struct _lv_anim_t lv_anim_t;
typedef struct _lv_event_t lv_event_t;
typedef struct _lv_indev_t lv_indev_t;
typedef int32_t lv_coord_t;
typedef unsigned char lv_opa_t;
typedef struct
{
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;
typedef void (*lv_anim_path_cb_t)(const lv_anim_t *, int32_t *);

/* Common LVGL constants used */
#define LV_PART_MAIN 0
#define LV_OPA_TRANSP 0
#define LV_OPA_COVER 255
#define LV_HOR_RES 480
#define LV_VER_RES 272
#define LV_EVENT_ALL 0
#define LV_EVENT_PRESSED 1
#define LV_EVENT_PRESSING 2
#define LV_EVENT_RELEASED 3
#define LV_EVENT_LEAVE 4
#define LV_OBJ_FLAG_SCROLLABLE (1u << 0)
#define LV_OBJ_FLAG_HIDDEN (1u << 1)

/* Function prototypes (stubs) to satisfy the compiler when LVGL isn't present) */
void *lv_mem_alloc(size_t size);
void lv_mem_free(void *p);
lv_obj_t *lv_obj_create(lv_obj_t *parent);
lv_obj_t *lv_scr_act(void);
void lv_obj_clear_flag(lv_obj_t *obj, uint32_t flag);
void lv_obj_add_flag(lv_obj_t *obj, uint32_t flag);
void lv_obj_set_user_data(lv_obj_t *obj, void *user);
void *lv_obj_get_user_data(lv_obj_t *obj);
void lv_obj_add_style(lv_obj_t *obj, const lv_style_t *style, uint32_t sel);
void lv_obj_move_foreground(lv_obj_t *obj);
void lv_obj_del_async(lv_obj_t *obj);
void lv_event_send(lv_obj_t *obj, int32_t code, void *param);
void lv_obj_add_event_cb(lv_obj_t *obj, void (*cb)(lv_event_t *), int32_t filter, void *user_data);
int32_t lv_obj_get_x(lv_obj_t *obj);
void lv_obj_set_x(lv_obj_t *obj, int32_t x);
int32_t lv_obj_get_y(lv_obj_t *obj);
void lv_obj_set_y(lv_obj_t *obj, int32_t y);
lv_opa_t lv_obj_get_style_bg_opa(lv_obj_t *obj, uint32_t part);
void lv_obj_set_style_bg_opa(lv_obj_t *obj, lv_opa_t opa, uint32_t part);
lv_indev_t *lv_indev_get_act(void);
void lv_indev_get_vect(lv_indev_t *indev, lv_point_t *point);
void lv_async_call(void (*cb)(void *), void *user_data);
void lv_anim_init(lv_anim_t *a);
void lv_anim_set_time(lv_anim_t *a, uint32_t time);
void lv_anim_set_path_cb(lv_anim_t *a, lv_anim_path_cb_t path);
void lv_anim_set_user_data(lv_anim_t *a, void *user_data);
void lv_anim_set_var(lv_anim_t *a, void *var);
void lv_anim_set_ready_cb(lv_anim_t *a, void (*cb)(lv_anim_t *));
void lv_anim_set_exec_cb(lv_anim_t *a, void (*cb)(void *, int32_t));
void lv_anim_set_values(lv_anim_t *a, int32_t start, int32_t end);
void lv_anim_start(lv_anim_t *a);
void lv_anim_del(void *var, void (*exec_cb)(void *, int32_t));
void *lv_anim_get_user_data(lv_anim_t *a);
#endif
#else
#include "lvgl/lvgl.h"
#endif

/* stdint/stdbool already included above */

/* Page switching animation type (mirror C++ enum order) */
typedef enum
{
    PM_LOAD_ANIM_GLOBAL = 0,
    PM_LOAD_ANIM_OVER_LEFT,
    PM_LOAD_ANIM_OVER_RIGHT,
    PM_LOAD_ANIM_OVER_TOP,
    PM_LOAD_ANIM_OVER_BOTTOM,
    PM_LOAD_ANIM_MOVE_LEFT,
    PM_LOAD_ANIM_MOVE_RIGHT,
    PM_LOAD_ANIM_MOVE_TOP,
    PM_LOAD_ANIM_MOVE_BOTTOM,
    PM_LOAD_ANIM_FADE_ON,
    PM_LOAD_ANIM_NONE,
    PM__LOAD_ANIM_LAST = PM_LOAD_ANIM_NONE
} pm_load_anim_t;

/* Page dragging direction */
typedef enum
{
    PM_ROOT_DRAG_DIR_NONE,
    PM_ROOT_DRAG_DIR_HOR,
    PM_ROOT_DRAG_DIR_VER,
} pm_root_drag_dir_t;

/* Animated setter/getter */
typedef void (*pm_lv_anim_setter_t)(void *, int32_t);
typedef int32_t (*pm_lv_anim_getter_t)(void *);

/* Animation switching record */
typedef struct
{
    struct
    {
        int32_t start;
        int32_t end;
    } enter;
    struct
    {
        int32_t start;
        int32_t end;
    } exit;
} pm_anim_value_t;

/* Page switching animation properties */
typedef struct
{
    pm_lv_anim_setter_t setter;
    pm_lv_anim_getter_t getter;
    pm_root_drag_dir_t dragDir;
    pm_anim_value_t push;
    pm_anim_value_t pop;
} pm_load_anim_attr_t;

/* Page lifecycle state */
typedef enum
{
    PM_PAGE_STATE_IDLE,
    PM_PAGE_STATE_LOAD,
    PM_PAGE_STATE_WILL_APPEAR,
    PM_PAGE_STATE_DID_APPEAR,
    PM_PAGE_STATE_ACTIVITY,
    PM_PAGE_STATE_WILL_DISAPPEAR,
    PM_PAGE_STATE_DID_DISAPPEAR,
    PM_PAGE_STATE_UNLOAD,
    PM__PAGE_STATE_LAST
} pm_page_state_t;

/* Anim attributes bound to a page/manager */
typedef struct
{
    uint8_t Type;  /* pm_load_anim_t */
    uint16_t Time; /* ms */
    lv_anim_path_cb_t Path;
} pm_anim_attr_t;

/* Stash data area */
typedef struct
{
    void *ptr;
    uint32_t size;
} pm_stash_t;

#define PM_PAGE_ANIM_TIME_DEFAULT ((uint16_t)500)
#ifdef lv_anim_path_ease_out
#define PM_PAGE_ANIM_PATH_DEFAULT (lv_anim_path_ease_out)
#else
#define PM_PAGE_ANIM_PATH_DEFAULT ((lv_anim_path_cb_t)0)
#endif

#endif /* PM_TYPES_H_ */
