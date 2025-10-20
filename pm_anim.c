#include "page_manager.h"
#include "page_manager_i.h"
#include "pm_log.h"
#include <string.h>

static void setter_x(void* obj, int32_t v){ lv_obj_set_x((lv_obj_t*)obj, v); }
static int32_t getter_x(void* obj){ return (int32_t)lv_obj_get_x((lv_obj_t*)obj); }
static void setter_y(void* obj, int32_t v){ lv_obj_set_y((lv_obj_t*)obj, v); }
static int32_t getter_y(void* obj){ return (int32_t)lv_obj_get_y((lv_obj_t*)obj); }
static void setter_opa(void* obj, int32_t v){ lv_obj_set_style_bg_opa((lv_obj_t*)obj, (lv_opa_t)v, LV_PART_MAIN); }
static int32_t getter_opa(void* obj){ return (int32_t)lv_obj_get_style_bg_opa((lv_obj_t*)obj, LV_PART_MAIN); }

/* Manager internals */
struct pm_manager;
static struct pm_manager* as_mgr(pm_manager_t* m){ return (struct pm_manager*)m; }

bool pm_get_is_over_anim(pm_manager_t* m, uint8_t anim)
{
    (void)m; return (anim >= PM_LOAD_ANIM_OVER_LEFT && anim <= PM_LOAD_ANIM_OVER_BOTTOM);
}

static bool pm_get_is_move_anim(uint8_t anim)
{
    return (anim >= PM_LOAD_ANIM_MOVE_LEFT && anim <= PM_LOAD_ANIM_MOVE_BOTTOM);
}

bool pm_anim_get_attr(uint8_t anim, pm_load_anim_attr_t* attr)
{
    lv_coord_t hor = LV_HOR_RES;
    lv_coord_t ver = LV_VER_RES;
    switch (anim) {
    case PM_LOAD_ANIM_OVER_LEFT:
        attr->dragDir = PM_ROOT_DRAG_DIR_HOR;
        attr->push.enter.start = hor; attr->push.enter.end = 0;
        attr->push.exit.start = 0;   attr->push.exit.end = 0;
        attr->pop.enter.start  = 0;  attr->pop.enter.end  = 0;
        attr->pop.exit.start   = 0;  attr->pop.exit.end   = hor;break;
    case PM_LOAD_ANIM_OVER_RIGHT:
        attr->dragDir = PM_ROOT_DRAG_DIR_HOR;
        attr->push.enter.start = -hor; attr->push.enter.end = 0;
        attr->push.exit.start = 0;    attr->push.exit.end = 0;
        attr->pop.enter.start  = 0;   attr->pop.enter.end  = 0;
        attr->pop.exit.start   = 0;   attr->pop.exit.end   = -hor;break;
    case PM_LOAD_ANIM_OVER_TOP:
        attr->dragDir = PM_ROOT_DRAG_DIR_VER;
        attr->push.enter.start = ver; attr->push.enter.end = 0;
        attr->push.exit.start = 0;    attr->push.exit.end = 0;
        attr->pop.enter.start  = 0;   attr->pop.enter.end  = 0;
        attr->pop.exit.start   = 0;   attr->pop.exit.end   = ver;break;
    case PM_LOAD_ANIM_OVER_BOTTOM:
        attr->dragDir = PM_ROOT_DRAG_DIR_VER;
        attr->push.enter.start = -ver; attr->push.enter.end = 0;
        attr->push.exit.start = 0;     attr->push.exit.end = 0;
        attr->pop.enter.start  = 0;    attr->pop.enter.end  = 0;
        attr->pop.exit.start   = 0;    attr->pop.exit.end   = -ver;break;
    case PM_LOAD_ANIM_MOVE_LEFT:
        attr->dragDir = PM_ROOT_DRAG_DIR_HOR;
        attr->push.enter.start = hor; attr->push.enter.end = 0;
        attr->push.exit.start = 0;   attr->push.exit.end = -hor;
        attr->pop.enter.start  = -hor;attr->pop.enter.end = 0;
        attr->pop.exit.start   = 0;   attr->pop.exit.end  = hor;break;
    case PM_LOAD_ANIM_MOVE_RIGHT:
        attr->dragDir = PM_ROOT_DRAG_DIR_HOR;
        attr->push.enter.start = -hor; attr->push.enter.end = 0;
        attr->push.exit.start = 0;    attr->push.exit.end = hor;
        attr->pop.enter.start  = hor;  attr->pop.enter.end = 0;
        attr->pop.exit.start   = 0;    attr->pop.exit.end  = -hor;break;
    case PM_LOAD_ANIM_MOVE_TOP:
        attr->dragDir = PM_ROOT_DRAG_DIR_VER;
        attr->push.enter.start = ver;  attr->push.enter.end = 0;
        attr->push.exit.start = 0;     attr->push.exit.end = -ver;
        attr->pop.enter.start  = -ver; attr->pop.enter.end = 0;
        attr->pop.exit.start   = 0;    attr->pop.exit.end  = ver;break;
    case PM_LOAD_ANIM_MOVE_BOTTOM:
        attr->dragDir = PM_ROOT_DRAG_DIR_VER;
        attr->push.enter.start = -ver; attr->push.enter.end = 0;
        attr->push.exit.start = 0;     attr->push.exit.end = ver;
        attr->pop.enter.start  = ver;  attr->pop.enter.end = 0;
        attr->pop.exit.start   = 0;    attr->pop.exit.end  = -ver;break;
    case PM_LOAD_ANIM_FADE_ON:
        attr->dragDir = PM_ROOT_DRAG_DIR_NONE;
        attr->push.enter.start = LV_OPA_TRANSP; attr->push.enter.end = LV_OPA_COVER;
        attr->push.exit.start  = LV_OPA_COVER;  attr->push.exit.end  = LV_OPA_COVER;
        attr->pop.enter.start  = LV_OPA_COVER;  attr->pop.enter.end  = LV_OPA_COVER;
        attr->pop.exit.start   = LV_OPA_COVER;  attr->pop.exit.end   = LV_OPA_TRANSP;break;
    case PM_LOAD_ANIM_NONE:
        memset(attr, 0, sizeof(*attr));
        return true;
    default:
        return false;
    }

    if (attr->dragDir == PM_ROOT_DRAG_DIR_HOR) {
        attr->setter = setter_x; attr->getter = getter_x;
    } else if (attr->dragDir == PM_ROOT_DRAG_DIR_VER) {
        attr->setter = setter_y; attr->getter = getter_y;
    } else {
        attr->setter = setter_opa; attr->getter = getter_opa;
    }
    return true;
}

void pm_anim_default_init(pm_manager_t* m, lv_anim_t* a)
{
    lv_anim_init(a);
    uint32_t time = (as_mgr(m)->anim.Current.Type == PM_LOAD_ANIM_NONE) ? 0u : as_mgr(m)->anim.Current.Time;
    lv_anim_set_time(a, time);
    lv_anim_set_path_cb(a, as_mgr(m)->anim.Current.Path ? as_mgr(m)->anim.Current.Path : lv_anim_path_ease_out);
}

/* Switch animation orchestration */
static void pm_on_switch_anim_finish(lv_anim_t* a)
{
    pm_page_t* base = (pm_page_t*)lv_anim_get_user_data(a);
    pm_manager_t* m = base->manager;
    extern void pm_state_update(pm_manager_t*, pm_page_t*);
    pm_state_update(m, base);
    base->priv.Anim.IsBusy = false;
    /* check finished */
    bool last_busy = (base->manager->prev && base->manager->prev->priv.Anim.IsBusy);
    if (!base->priv.Anim.IsBusy && !last_busy) {
        as_mgr(m)->anim.IsSwitchReq = false;
        as_mgr(m)->prev = as_mgr(m)->current;
        if (!as_mgr(m)->anim.IsEntering) {
            as_mgr(m)->anim.Current = as_mgr(m)->anim.Global;
        }
    }
}

void pm_switch_anim_create(pm_manager_t* m, pm_page_t* base)
{
    pm_load_anim_attr_t attr;
    if (!pm_anim_get_attr(as_mgr(m)->anim.Current.Type, &attr)) return;
    lv_anim_t a;
    pm_anim_default_init(m, &a);
    lv_anim_set_user_data(&a, base);
    lv_anim_set_var(&a, base->root);
    lv_anim_set_ready_cb(&a, pm_on_switch_anim_finish);
    lv_anim_set_exec_cb(&a, attr.setter);
    int32_t start = attr.getter ? attr.getter(base->root) : 0;
    if (as_mgr(m)->anim.IsEntering) {
        if (base->priv.Anim.IsEnter) lv_anim_set_values(&a, attr.push.enter.start, attr.push.enter.end);
        else lv_anim_set_values(&a, start, attr.push.exit.end);
    } else {
        if (base->priv.Anim.IsEnter) lv_anim_set_values(&a, attr.pop.enter.start, attr.pop.enter.end);
        else lv_anim_set_values(&a, start, attr.pop.exit.end);
    }
    lv_anim_start(&a);
    base->priv.Anim.IsBusy = true;
}
