#include "page_manager.h"
#include "page_manager_i.h"
#include "pm_log.h"

#define PM_INDEV_DEF_DRAG_THROW 20

static void pm_root_get_drag_predict(lv_coord_t* x, lv_coord_t* y)
{
    lv_indev_t* indev = lv_indev_get_act();
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    lv_coord_t yp = 0, xp = 0;
    while (vect.y != 0) { yp += vect.y; vect.y = vect.y * (100 - PM_INDEV_DEF_DRAG_THROW) / 100; }
    while (vect.x != 0) { xp += vect.x; vect.x = vect.x * (100 - PM_INDEV_DEF_DRAG_THROW) / 100; }
    *x = xp; *y = yp;
}

static void pm_on_root_drag_anim_finish(lv_anim_t* a)
{
    pm_manager_t* m = (pm_manager_t*)lv_anim_get_user_data(a);
    ((struct pm_manager*)m)->anim.IsBusy = false;
    extern pm_page_t* pm_stack_top_after(struct pm_manager*);
    pm_page_t* bottom = pm_stack_top_after((struct pm_manager*)m);
    if (bottom) lv_obj_add_flag(bottom->root, LV_OBJ_FLAG_HIDDEN);
}

static void pm_on_root_async_leave(void* data)
{
    pm_page_t* base = (pm_page_t*)data;
    lv_obj_send_event(base->root, LV_EVENT_LEAVE, base);
}

static void pm_on_root_drag_event(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (!(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED)) return;
    lv_obj_t* root = lv_event_get_current_target(event);
    pm_page_t* base = (pm_page_t*)lv_event_get_user_data(event);
    if (!base) return;
    pm_manager_t* m = base->manager;
    pm_load_anim_attr_t attr;
    if (!pm_get_current_anim_attr(m, &attr)) return;

    if (code == LV_EVENT_PRESSED) {
        if (((struct pm_manager*)m)->anim.IsSwitchReq) return;
        if (!((struct pm_manager*)m)->anim.IsBusy) return;
        lv_anim_del(root, attr.setter);
        ((struct pm_manager*)m)->anim.IsBusy = false;
        extern pm_page_t* pm_stack_top_after(struct pm_manager*);
        pm_page_t* bottom = pm_stack_top_after((struct pm_manager*)m);
        lv_obj_clear_flag(bottom->root, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_PRESSING) {
        lv_coord_t cur = attr.getter(root);
        lv_coord_t max = (attr.pop.exit.start > attr.pop.exit.end) ? attr.pop.exit.start : attr.pop.exit.end;
        lv_coord_t min = (attr.pop.exit.start < attr.pop.exit.end) ? attr.pop.exit.start : attr.pop.exit.end;
        lv_point_t offset; lv_indev_get_vect(lv_indev_get_act(), &offset);
        if (attr.dragDir == PM_ROOT_DRAG_DIR_HOR) cur += offset.x; else if (attr.dragDir == PM_ROOT_DRAG_DIR_VER) cur += offset.y;
        if (cur < min) cur = min; if (cur > max) cur = max;
        attr.setter(root, cur);
    } else if (code == LV_EVENT_RELEASED) {
        if (((struct pm_manager*)m)->anim.IsSwitchReq) return;
        lv_coord_t offset_sum = attr.push.enter.end - attr.push.enter.start;
        lv_coord_t xp = 0, yp = 0; pm_root_get_drag_predict(&xp, &yp);
        lv_coord_t start = attr.getter(root), end = start;
        if (attr.dragDir == PM_ROOT_DRAG_DIR_HOR) end += xp; else if (attr.dragDir == PM_ROOT_DRAG_DIR_VER) end += yp;
        if ((end > 0 ? end : -end) > ((offset_sum > 0 ? offset_sum : -offset_sum) / 2)) {
            lv_async_call(pm_on_root_async_leave, base);
        } else if (end != attr.push.enter.end) {
            ((struct pm_manager*)m)->anim.IsBusy = true;
            lv_anim_t a; extern void pm_anim_default_init(pm_manager_t*, lv_anim_t*);
            pm_anim_default_init(m, &a);
            lv_anim_set_user_data(&a, m);
            lv_anim_set_var(&a, root);
            lv_anim_set_values(&a, start, attr.push.enter.end);
            lv_anim_set_exec_cb(&a, attr.setter);
            lv_anim_set_ready_cb(&a, pm_on_root_drag_anim_finish);
            lv_anim_start(&a);
        }
    }
}

void pm_root_enable_drag(pm_manager_t* m, lv_obj_t* root)
{
    (void)m;
    pm_page_t* base = (pm_page_t*)lv_obj_get_user_data(root);
    lv_obj_add_event_cb(root, pm_on_root_drag_event, LV_EVENT_ALL, base);
}
