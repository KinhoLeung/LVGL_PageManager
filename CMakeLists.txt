cmake_minimum_required(VERSION 3.16)
project(lvgl_page_manager LANGUAGES C)

add_library(lvgl_page_manager STATIC
    PageBase.c
    PageManager.c
    PM_Anim.c
    PM_Drag.c
)
add_library(lvgl_page_manager::lvgl_page_manager ALIAS lvgl_page_manager)
target_include_directories(lvgl_page_manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(lvgl_page_manager PUBLIC c_std_99)
set_target_properties(lvgl_page_manager PROPERTIES
    C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)

# The application supplies LVGL and its lv_conf.h; do not vendor LVGL here.
if(TARGET lvgl::lvgl)
    target_link_libraries(lvgl_page_manager PUBLIC lvgl::lvgl)
elseif(TARGET lvgl)
    target_link_libraries(lvgl_page_manager PUBLIC lvgl)
elseif(TARGET idf::lvgl)
    target_link_libraries(lvgl_page_manager PUBLIC idf::lvgl)
endif()
