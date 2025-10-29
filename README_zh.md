[English](README.en.md) | [中文](README.zh.md)

# LVGL PageManager (C)

LVGL_PageManager 是面向 LVGL v9 的 PageManager 架构纯 C 实现，可直接嵌入传统 C 固件工程，同时保留原版 C++ 模块的页面生命周期与导航能力。

## 功能亮点
- 栈式页面导航：使用 `pm_manager_*` 接口安装、注册、压栈、弹栈、替换，以及一键回到首页。
- 完整生命周期回调，从 `onViewLoad` 到 `onViewDidUnload`，通过轻量级 vtable 组织代码。
- 灵活的页面缓存策略，并提供 Stash 缓冲区在页面之间传递小型状态数据。
- 可配置的切换动画：支持全局默认、单页覆盖，以及在覆盖类动画下的边缘拖拽手势。
- 工厂接口让页面按类名动态构建，避免在业务层硬编码构造流程。
- 自带日志封装 `pm_log.h`，默认静默，亦可接入现有日志系统。

## 快速上手
1. 将源码（`page_manager*.c/h`、`page_base*.c/h`、`pm_*`）加入 LVGL 工程并参与编译。
2. 在需要控制导航的源文件中包含 `page_manager.h`。
3. 确保工程能够包含 `lvgl.h`（或 `lvgl/lvgl.h`）。仓库内的最小桩仅用于静态检查，实际构建必须链接 LVGL。

### 定义页面
实现所需的生命周期回调，并通过 `pm_page_create` 生成页面对象。

```c
static void home_on_view_load(pm_page_t *page) {
    page->root = lv_obj_create(lv_scr_act());
    // 在这里构建界面
}

static const pm_page_vtable_t HOME_VTABLE = {
    .onViewLoad = home_on_view_load,
    .onViewDidLoad = NULL,
    .onViewWillAppear = NULL,
    .onViewDidAppear = NULL,
    .onViewWillDisappear = NULL,
    .onViewDidDisappear = NULL,
    .onViewUnload = NULL,
    .onViewDidUnload = NULL,
};

pm_page_t *home_page_create(void) {
    return pm_page_create(&HOME_VTABLE);
}
```

如需个性化动画或缓存策略，可在 `onCustomAttrConfig` 中调用 `pm_page_set_custom_cache_enable`、`pm_page_set_custom_auto_cache_enable`、`pm_page_set_custom_load_anim`。

### 接入工厂
通过工厂回调集中管理页面构建逻辑，便于管理和扩展。

```c
static pm_page_t *factory_create(const char *class_name, void *user_ctx) {
    if (strcmp(class_name, "HomePage") == 0) return home_page_create();
    if (strcmp(class_name, "SettingsPage") == 0) return settings_page_create();
    return NULL;
}

static const pm_factory_t PAGE_FACTORY = {
    .create = factory_create,
    .destroy = NULL,   // 使用默认 pm_page_destroy
    .user = NULL,
};
```

### 驱动导航
- 使用 `pm_manager_install` / `pm_manager_uninstall` 动态增删页面类。
- 使用 `pm_manager_register` 注册预先构造好的页面实例。
- `pm_manager_push`、`pm_manager_pop`、`pm_manager_replace`、`pm_manager_back_home` 控制页面栈。
- `pm_manager_set_global_load_anim` 配置全局动画，`pm_page_set_custom_load_anim` 覆盖单页动画。
- `pm_page_stash_pop` 在目标页内提取随导航传入的 `pm_stash_t` 数据。

### 共享状态：Stash
在 `pm_manager_push` 或 `pm_manager_replace` 时传入 `pm_stash_t`，可以复制小型结构体（如配置、标识符等）。目标页面通过 `pm_page_stash_pop(page, &buffer, sizeof(buffer))` 一次性接收并释放缓存。

### 动画与拖拽
默认动画为 `PM_LOAD_ANIM_OVER_LEFT`。可选覆盖类、滑动类、淡入类或禁用动画。覆盖类动画自动开启边缘拖拽，用户可中途取消或完成页面切换。

### 日志
`pm_log.h` 中的 `PM_LOG_INFO/WARN/ERROR` 默认为空实现。若希望输出日志，可在包含头文件前自定义这些宏。

## 目录结构
- `page_manager*.c/h`、`pm_router*.c`、`pm_state.c`：页面管理与生命周期核心逻辑。
- `pm_anim*.c/h`：切换动画、缓动与拖拽手势。
- `page_base*.c/h`：页面对象、vtable 与 stash 工具。
- `pm_factory.h`：页面工厂接口定义。
- `kvec.h`：轻量级动态数组实现，用于页面池与栈。

## 示例
参见 `examples/demo.c`，其中包含完整的两页导航示例，演示了工厂创建、Stash 数据传递与动画配置。

## 后续规划建议
- 提供面向纯 C 结构体的便捷注册辅助函数。
- 发布结合 LVGL 官方示例的演示工程。
- 扩展动画曲线，覆盖更多界面效果。

## 许可证
当前仓库尚未声明许可证。开源前请补充许可文件，明确他人使用权限。
