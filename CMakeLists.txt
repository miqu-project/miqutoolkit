cmake_minimum_required(VERSION 3.20)
project(miqutoolkit VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_CURSOR REQUIRED IMPORTED_TARGET wayland-cursor)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols>=1.24)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(GDK_PIXBUF REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")

# Generate xdg-shell protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
    DEPENDS ${XDG_SHELL_XML}
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.c
    DEPENDS ${XDG_SHELL_XML}
)

# Generate wlr-layer-shell protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml
)

# Generate wlr-foreign-toplevel-management protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml
)

# Generate ext-workspace protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-workspace-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-workspace-v1.xml
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-workspace-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-workspace-v1.xml
)

# Generate ext-idle-notify protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml
)

# Generate ext-session-lock protocol
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.h
    COMMAND ${WAYLAND_SCANNER} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.c
    COMMAND ${WAYLAND_SCANNER} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml
)

# Shared Library
add_library(miqutoolkit SHARED
    src/core/app_engine.cpp
    src/core/window.cpp
    src/core/config.cpp
    src/core/shm_pool.cpp
    src/view/view_group.cpp
    src/view/linear_layout.cpp
    src/view/frame_layout.cpp
    src/view/relative_layout.cpp
    src/view/card_view.cpp
    src/view/text_view.cpp
    src/view/button.cpp
    src/view/image_view.cpp
    src/view/edit_text.cpp
    src/view/search_view.cpp
    src/view/grid_view.cpp
    src/view/slider.cpp
    src/view/progress_bar.cpp
    src/view/image_button.cpp
    src/view/grid_item_view.cpp
    src/view/list_view.cpp
    src/view/list_item_view.cpp
    src/view/divider_view.cpp
    src/view/tab_bar.cpp
    src/view/view_pager.cpp
    src/view/navigation_view.cpp
    src/view/resizable_container.cpp
    src/view/scroll_view.cpp
    src/view/switch.cpp
    src/view/toolbar.cpp
    src/view/bottom_navigation_view.cpp
    src/view/spinner.cpp
    src/view/popup_window.cpp
    src/view/popup_menu.cpp
    src/system/window_manager.cpp
    src/system/workspace_manager.cpp
    src/system/output_manager.cpp
    src/system/app_manager.cpp
    src/system/idle_manager.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/wlr-layer-shell-unstable-v1-client-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/wlr-foreign-toplevel-management-unstable-v1-client-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ext-workspace-v1-client-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.c
    ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.c
)

set_target_properties(miqutoolkit PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_include_directories(miqutoolkit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${PANGOCAIRO_INCLUDE_DIRS}
        ${GDK_PIXBUF_INCLUDE_DIRS}
)

target_link_libraries(miqutoolkit
    PUBLIC
        PkgConfig::WAYLAND_CLIENT
        PkgConfig::WAYLAND_CURSOR
        PkgConfig::XKBCOMMON
        PkgConfig::CAIRO
        PkgConfig::PANGOCAIRO
        PkgConfig::GDK_PIXBUF
)

target_compile_options(miqutoolkit PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
)

# Generate pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/miqutoolkit.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miqutoolkit.pc
    @ONLY
)

# Installation Rules
install(TARGETS miqutoolkit
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/miqutoolkit
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miqutoolkit.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(FILES assets/miqutoolkit.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/miqutoolkit
)
