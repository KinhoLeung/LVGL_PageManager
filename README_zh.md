[English](README.md) | [中文](README_zh.md)

# LVGL_PageManager

从 X-TRACK 的 `Software/X-Track/USER/App/Utils/PageManager` 改写的独立 **C99 / LVGL 9.x** 页面管理组件。使用回调表代替 C++ 继承和虚函数，用链表维护页面池与导航栈，不依赖 STL、Arduino、X-TRACK 的业务代码或额外任务。

支持页面注册、可选工厂、Push / Pop / Replace / BackHome、生命周期、缓存策略、参数深拷贝、页面动画和拖动返回。仅支持 LVGL 9.x，编译时拒绝其他主版本。

## 文件与集成

| 文件 | 用途 |
| --- | --- |
| `PageManager.h` | 管理器、注册和导航接口 |
| `PageBase.h` | 页面回调、属性、缓存与参数接口 |
| `PageFactory.h` | 可选的页面工厂接口 |
| `PageManager.c` | 页面池、导航栈和生命周期 |
| `PageBase.c` | 页面属性与 stash |
| `PM_Anim.c` | 切换动画 |
| `PM_Drag.c` | 拖动返回与回弹 |
| `PM_Private.h` | 内部结构，不应由应用直接使用 |
| `examples/` | 可编译的主页 / 详情页示例 |
| `tests/` | 使用真实 LVGL 的无窗口集成测试 |

手动集成时，将上面四个 `.c` 文件加入工程，把本目录加入头文件搜索路径，并链接应用已有的 LVGL 9。默认 `#include "lvgl.h"`，其他布局可通过编译宏设置，例如 `PM_LVGL_INCLUDE="lvgl/lvgl.h"`。`lv_conf.h` 和显示、输入驱动仍由应用提供。

CMake 工程可直接接入：

```cmake
add_subdirectory(path/to/lvgl)             # 应用已有 LVGL target 时省略
add_subdirectory(path/to/LVGL_PageManager)
target_link_libraries(your_app PRIVATE lvgl_page_manager::lvgl_page_manager)
```

自动识别 `lvgl::lvgl`、`lvgl` 和 `idf::lvgl` target；自定义 LVGL target 可自行链接到 `lvgl_page_manager`。

## 最小使用示例

```c
#include "PageManager.h"

static void home_load(pm_page_t * page)
{
    lv_obj_t * label = lv_label_create(pm_page_get_root(page));
    lv_label_set_text(label, "Home");
    lv_obj_center(label);
}

static void details_load(pm_page_t * page)
{
    lv_obj_t * label = lv_label_create(pm_page_get_root(page));
    lv_label_set_text(label, "Details");
    lv_obj_center(label);
}

static pm_manager_t * manager;

void app_ui_init(void)
{
    /* 先完成 lv_init()、显示驱动初始化。描述符会被组件复制。 */
    pm_page_descriptor_t home = {
        .callbacks = { .on_view_load = home_load }
    };
    pm_page_descriptor_t details = {
        .callbacks = { .on_view_load = details_load }
    };

    manager = pm_manager_create(NULL);  /* NULL 表示当前活动 screen */
    if(!manager) return;
    if(!pm_manager_register(manager, "home", &home) ||
       !pm_manager_register(manager, "details", &details) ||
       !pm_manager_push(manager, "home", NULL)) {
        pm_manager_delete(manager);
        manager = NULL;
    }
}

/* 在按钮事件或其他 LVGL 上下文中调用，检查返回值。 */
bool app_open_details(void)
{
    return pm_manager_push(manager, "details", NULL);
}

bool app_back(void)
{
    return pm_manager_pop(manager);
}

void app_ui_deinit(void)
{
    if(manager && pm_manager_delete(manager)) manager = NULL;
}
```

完整可交互示例见 [`examples/page_manager_example.c`](examples/page_manager_example.c)。将它加入工程，在显示初始化后调用 `pm_example_create(NULL)`，保留返回的管理器用于销毁。

## 生命周期与回调

所有回调均为 `void callback(pm_page_t * page)`，可选字段置 `NULL`。

```text
注册：on_custom_attr_config

首次进入 / 重新加载：
on_view_load -> on_view_did_load -> on_view_will_appear
             -> [动画] -> on_view_did_appear

离开：
on_view_will_disappear -> [动画] -> on_view_did_disappear
    -> 保留 root，等待再次出现
    或 on_view_unload -> 删除 root -> on_view_did_unload

从缓存返回：on_view_will_appear -> [动画] -> on_view_did_appear
```

切换双方的顺序为：旧页 `will_disappear`，新页加载（如果需要）和 `will_appear`，动画，旧页 `did_disappear` / 卸载，新页 `did_appear`。注册时触发属性配置回调；`on_view_load` 中 root 已经创建，`on_view_did_unload` 中 root 已经变为 `NULL`。

回调中通过 `pm_page_get_user_data()` 访问自己的上下文，可设置页面缓存、动画、ID 等属性。页面创建的定时器、订阅和业务资源需由页面在相应的消失或卸载回调中释放。组件负责删除 root 及其所有子对象。

**所有调用必须在 LVGL 线程或应用的 LVGL 锁内完成。** 动画、拖动和生命周期回调期间，导航、注册、注销及管理器配置操作返回失败，防止重入。`did_appear` 执行期间同样保持 busy；需要从生命周期中自动跳页时，可用应用定时器或 `lv_async_call()` 延后，并由应用管理这些任务的取消与寿命。

## 导航与所有权

| 接口 | 行为 |
| --- | --- |
| `pm_manager_register` | 复制名字、描述符并返回组件拥有的页面句柄 |
| `pm_manager_push` | 将已注册且尚未入栈的页面压栈 |
| `pm_manager_pop` | 弹出栈顶并返回下一页，保留最后一页 |
| `pm_manager_replace` | 替换栈顶，强制卸载被替换的页面，栈深不变 |
| `pm_manager_back_home` | 返回栈底，强制卸载所有被移除页面，对顶页执行返回动画 |
| `pm_manager_clear` | 清空栈和全部已缓存 root，保留页面注册信息 |
| `pm_manager_unregister` / `uninstall` | 拒绝仍在栈中的页面；卸载缓存并释放句柄 |
| `pm_manager_delete` | 结束动画、清空页面并释放管理器 |

同一个页面不能重复入栈；名字不能为空或重复。非法请求返回 `false` / `NULL`，不改变导航栈。空栈或仅剩首页时 `pop` / `back_home` 返回 `false`，`replace` 要求栈非空。

`clear` / `delete` 可以在切换动画或拖动过程中调用；已接受的切换会先完成生命周期，再清理资源。它们不能在生命周期回调或组件正在执行的 LVGL 对象事件中重入调用。页面句柄在注销 / 删除后失效，不应由应用 `free`。

组件为每个管理器创建一个容器，其下每页一个 root，均默认填满父对象。动画距离使用管理器容器的实际内容尺寸，支持多个管理器和自定义父容器。组件保留 page root 的坐标、大小、隐藏标志和整体透明度用于管理；应用应在 root 内创建自己的布局容器。`pm_manager_set_root_style()` 借用样式指针，仅应用于后续新建的页面 root；样式需保持有效，直到这些 root 被删除。

**必须在删除父 screen / display 或 `lv_deinit()` 之前调用 `pm_manager_delete()`。** 不要直接删除管理器容器或页面 root。直接注册时，`user_data` 的内存由应用管理；回调表和名字可放在局部变量中，因为组件会复制它们。

`pm_manager_get_previous()` 获取栈顶之下的页面；`pm_manager_get_previous_name()` 获取最近一次（或正在进行的）切换的来源页名，无来源时返回 `NULL`。这两个概念是独立的。

## 缓存

```c
pm_page_set_cache_mode(page, PM_CACHE_AUTO);  /* 默认 */
pm_page_set_cache_mode(page, PM_CACHE_KEEP);
pm_page_set_cache_mode(page, PM_CACHE_NONE);
```

| 策略 | 被新页覆盖时 | Pop 后 |
| --- | --- | --- |
| `PM_CACHE_AUTO` | 保留 root | 卸载 root |
| `PM_CACHE_KEEP` | 保留 root | 保留 root，可再次 Push 复用 |
| `PM_CACHE_NONE` | 卸载 root | 卸载 root |

`PM_CACHE_NONE` 的页面可留在导航栈中，返回时重新加载。`Replace`、`BackHome`、`Clear` 和销毁会强制卸载被移除的页面，即使其策略是 `KEEP`。缓存设置决定下一次消失时如何处理，不会立即卸载一个已经隐藏的缓存页。

## 参数传递（stash）

```c
typedef struct { uint32_t item_id; } details_args_t;
details_args_t args = { 123 };
pm_stash_t stash = PM_STASH_MAKE(args);
bool opened = pm_manager_push(manager, "details", &stash);

/* 在目标页面的 on_view_will_appear 中读取： */
details_args_t received;
if(pm_page_stash_pop(page, &received, sizeof(received))) {
    /* 使用 received.item_id */
}
```

导航先分配和复制参数，再提交栈变更；调用者的数据在函数返回后即可释放。新参数替换此前未消费的参数，大小可以变化。Push / Replace 传 `NULL` 会清除旧参数，Pop / BackHome 不提供新的参数。

`pm_page_stash_pop()` 要求大小完全相等，成功后释放参数；失败不消费。`pm_page_stash_peek()` 可查询内容与大小，返回指针有效期截至下一次参数更新、消费、清空或页面卸载。组件卸载时释放未消费参数。传递的数据应自包含；结构体里的指针不会进一步深拷贝。

## 动画与拖动返回

```c
pm_manager_set_animation(manager, PM_ANIM_OVER_LEFT, 300, lv_anim_path_ease_out);
pm_page_set_animation(page, PM_ANIM_MOVE_TOP, 200, lv_anim_path_linear);
pm_page_set_animation(page, PM_ANIM_GLOBAL, 0, NULL); /* 恢复继承 */
pm_manager_set_drag(manager, true, 50);               /* 默认配置 */
```

支持 `OVER_LEFT/RIGHT/TOP/BOTTOM`、`MOVE_LEFT/RIGHT/TOP/BOTTOM`、`FADE_ON`、`NONE`。方向描述新页进入时的移动方向，例如 `OVER_LEFT` 从右侧进入、向右拖动返回。`NULL` 曲线使用 `lv_anim_path_ease_out`；时长单位毫秒，范围 `0..INT32_MAX`。全局动画不可设置成 `GLOBAL`。

每次 Push / Replace 保存该次进入动画的配置，Pop 按保存的配置反向播放，所以全局配置后续变更不会改变已经入栈页面的返回方向。`NONE` 或时长 `0` 同步完成生命周期，不创建零时长动画。淡入淡出控制整个 root 及子树的透明度。

拖动默认启用，只适用于 `OVER_*` 且下层页面 root 已加载的情况。拖动 root 的空白区域会暂时露出下层页面，释放时结合位移和惯性判断是否达到阈值：达到则执行 Pop，否则回弹。暂时露出不会调用下层页面的出现回调。按钮、列表等子控件保留自己的事件；不对其冒泡事件启动返回拖动。`PRESS_LOST` 会回弹。阈值允许 `1..100`。

## 可选工厂

```c
#include <string.h>

static bool create_page(const char * class_name, const char * instance_name,
                        pm_page_descriptor_t * result, void * factory_data)
{
    (void)instance_name;
    (void)factory_data;
    if(strcmp(class_name, "HomePage") != 0) return false;
    *result = (pm_page_descriptor_t) {
        .callbacks = { .on_view_load = home_load }
    };
    return true;
}

pm_page_factory_t factory = { .create = create_page };
pm_manager_set_factory(manager, &factory);
pm_page_t * home = pm_manager_install(manager, "HomePage", "home");
```

工厂通过 `create` 填充描述符；`instance_name == NULL` 时使用类名。若工厂分配页面上下文，可提供 `destroy(page_user_data, factory_user_data)`，在卸载注册信息或删除管理器时释放它。工厂成功但注册分配失败时，同样调用 `destroy`；工厂返回失败时应自行清理其临时资源。每个安装的页面保存当时的销毁回调与工厂上下文，后续更换工厂不影响它们。该上下文必须活到对应页面卸载注册信息之后。

## 验证

以下命令在本仓库根目录执行。

测试不使用 LVGL mock；由真实显示对象、动画定时器和输入设备驱动页面管理器。

已在 **LVGL 9.0.0、9.2.2、9.5.0** 上完成编译与集成测试。组件及示例采用 C99，测试编译启用 `-Wall -Wextra -Werror -pedantic`。

```sh
cmake -S tests -B build \
  -DLVGL_DIR=/absolute/path/to/lvgl-9.x
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Windows MinGW 可在配置命令中增加 `-G "MinGW Makefiles"`。测试用的 `tests/lv_conf.h` 只用于独立测试，应用应使用自己的配置。

测试覆盖导航和生命周期顺序、三种缓存策略、参数复制和长度变化、参数分配失败时的栈恢复、全部动画、反向动画配置、busy 与回调重入、动画取消、切换中清空 / 销毁、工厂上下文释放、四个方向的真实输入拖动、重复创建销毁的内存核对以及示例按钮导航。

LVGL API 参考：[动画](https://docs.lvgl.io/9.2/overview/animations.html)、[对象](https://docs.lvgl.io/9.2/API/core/lv_obj.html)。组件沿用原 PageManager 的 MIT 许可证，见 [LICENSE](LICENSE)。
