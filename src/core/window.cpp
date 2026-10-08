#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/view/popup_window.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/system/window_manager.hpp"
#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "ext-session-lock-v1-client-protocol.h"
#include <wayland-cursor.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace miqu {

const struct ext_session_lock_surface_v1_listener Window::s_lock_surface_listener = {
    .configure = [](void* data, struct ext_session_lock_surface_v1* surface, uint32_t serial, uint32_t width, uint32_t height) {
        auto* self = static_cast<Window*>(data);
        ext_session_lock_surface_v1_ack_configure(surface, serial);

        if (width > 0 && height > 0) {
            self->m_width = width;
            self->m_height = height;
        }

        if (!self->m_shm_pool) {
            self->m_shm_pool = std::make_unique<ShmPool>(AppEngine::instance()->get_shm(), self->m_width, self->m_height);
        } else {
            self->m_shm_pool->resize(self->m_width, self->m_height);
        }

        self->m_configured = true;
        self->schedule_redraw();
    }
};

const struct xdg_surface_listener Window::s_xdg_surface_listener = {
    .configure = [](void* data, struct xdg_surface* surface, uint32_t serial) {
        auto* self = static_cast<Window*>(data);
        xdg_surface_ack_configure(surface, serial);

        if (!self->m_shm_pool) {
            self->m_shm_pool = std::make_unique<ShmPool>(AppEngine::instance()->get_shm(), self->m_width, self->m_height);
        } else {
            self->m_shm_pool->resize(self->m_width, self->m_height);
        }

        self->m_configured = true;
        self->schedule_redraw();
    }
};

const struct xdg_toplevel_listener Window::s_xdg_toplevel_listener = {
    .configure = [](void* data, struct xdg_toplevel*, int32_t width, int32_t height, struct wl_array*) {
        auto* self = static_cast<Window*>(data);
        if (width > 0 && height > 0) {
            self->m_width = width;
            self->m_height = height;
        }
    },
    .close = [](void* data, struct xdg_toplevel*) {
        auto* self = static_cast<Window*>(data);
        if (self->m_on_close) {
            self->m_on_close();
        }
        self->close();
    },
    .configure_bounds = [](void*, struct xdg_toplevel*, int32_t, int32_t) {},
    .wm_capabilities = [](void*, struct xdg_toplevel*, struct wl_array*) {}
};

const struct zwlr_layer_surface_v1_listener Window::s_layer_surface_listener = {
    .configure = [](void* data, struct zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t w, uint32_t h) {
        auto* self = static_cast<Window*>(data);
        zwlr_layer_surface_v1_ack_configure(surface, serial);

        if (w > 0 && h > 0) {
            self->m_width = w;
            self->m_height = h;
        }

        if (!self->m_shm_pool) {
            self->m_shm_pool = std::make_unique<ShmPool>(AppEngine::instance()->get_shm(), self->m_width, self->m_height);
        } else {
            self->m_shm_pool->resize(self->m_width, self->m_height);
        }

        self->m_configured = true;
        self->schedule_redraw();
    },
    .closed = [](void* data, struct zwlr_layer_surface_v1* surface) {
        auto* self = static_cast<Window*>(data);
        if (self->m_on_close) {
            self->m_on_close();
        }
        self->close();
    }
};

void Window::update_seat_capabilities(uint32_t caps) {
    auto* engine = AppEngine::instance();
    if (!engine || !engine->get_seat()) return;
    struct wl_seat* seat = engine->get_seat();

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !m_pointer) {
        m_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(m_pointer, &s_pointer_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && m_pointer) {
        wl_pointer_destroy(m_pointer);
        m_pointer = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !m_keyboard) {
        m_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(m_keyboard, &s_keyboard_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && m_keyboard) {
        wl_keyboard_destroy(m_keyboard);
        m_keyboard = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !m_touch) {
        m_touch = wl_seat_get_touch(seat);
        wl_touch_add_listener(m_touch, &s_touch_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && m_touch) {
        wl_touch_destroy(m_touch);
        m_touch = nullptr;
    }
}

const struct wl_pointer_listener Window::s_pointer_listener = {
    .enter = [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {
        auto* self = static_cast<Window*>(data);
        self->m_last_x = wl_fixed_to_double(sx);
        self->m_last_y = wl_fixed_to_double(sy);
        self->update_cursor(serial);
        if (self->m_popup_view) {
            self->m_popup_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_popup_bounds);
            self->schedule_redraw();
        } else if (self->m_root_view) {
            self->m_root_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_allocated_content_bounds);
            self->schedule_redraw();
        }
    },
    .leave = [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface) {},
    .motion = [](void* data, struct wl_pointer*, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
        auto* self = static_cast<Window*>(data);
        self->m_last_x = wl_fixed_to_double(sx);
        self->m_last_y = wl_fixed_to_double(sy);
        if (self->m_popup_view) {
            if (self->m_popup_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_popup_bounds)) {
                self->schedule_redraw();
            }
            return;
        }
        if (self->m_root_view) {
            if (self->m_root_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_allocated_content_bounds)) {
                self->schedule_redraw();
            }
        }
    },
    .button = [](void* data, struct wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
        auto* self = static_cast<Window*>(data);
        MouseButton mb = MouseButton::Left;
        if (button == 0x111) mb = MouseButton::Right;
        else if (button == 0x112) mb = MouseButton::Middle;
        else if (button == 0x113 || button == 0x116) mb = MouseButton::Back;
        else if (button == 0x114 || button == 0x115) mb = MouseButton::Forward;

        bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

        if (self->m_popup_view) {
            if (pressed && !self->m_popup_bounds.contains(self->m_last_x, self->m_last_y)) {
                if (!self->m_active_popup || self->m_active_popup->is_dismiss_on_outside_click()) {
                    self->dismiss_popup();
                    return;
                }
            }
            if (self->m_popup_view->on_mouse_button(self->m_last_x, self->m_last_y, mb, pressed, self->m_popup_bounds)) {
                self->schedule_redraw();
            }
            return;
        }

        if (pressed && self->m_close_on_click_outside && !self->m_allocated_content_bounds.contains(self->m_last_x, self->m_last_y)) {
            if (self->m_on_close) self->m_on_close();
            self->close();
            return;
        }

        View* prev_focused = self->m_focused_view;

        if (self->m_root_view) {
            if (self->m_root_view->on_mouse_button(self->m_last_x, self->m_last_y, mb, pressed, self->m_allocated_content_bounds)) {
                self->schedule_redraw();
            }
        }

        if (pressed && mb == MouseButton::Left && prev_focused && self->m_focused_view == prev_focused) {
            if (!prev_focused->contains_point(self->m_last_x, self->m_last_y)) {
                self->clear_focus();
            }
        }
    },
    .axis = [](void* data, struct wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value) {
        auto* self = static_cast<Window*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
            double delta = wl_fixed_to_double(value);
            if (self->m_popup_view) {
                if (self->m_popup_view->on_scroll(delta)) {
                    self->schedule_redraw();
                }
                return;
            }
            if (self->m_root_view) {
                if (self->m_root_view->on_scroll(delta)) {
                    self->schedule_redraw();
                }
            }
        }
    },
    .frame = [](void*, struct wl_pointer*) {},
    .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
    .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
#ifdef WL_POINTER_WARP_SINCE_VERSION
    .warp = [](void*, struct wl_pointer*, wl_fixed_t, wl_fixed_t) {},
#endif
};

const struct wl_keyboard_listener Window::s_keyboard_listener = {
    .keymap = [](void* data, struct wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
        auto* self = static_cast<Window*>(data);
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            ::close(fd);
            return;
        }

        char* map_shm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        if (map_shm == MAP_FAILED) {
            ::close(fd);
            return;
        }

        if (!self->m_xkb_ctx) {
            self->m_xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        }

        if (self->m_xkb_keymap) xkb_keymap_unref(self->m_xkb_keymap);
        self->m_xkb_keymap = xkb_keymap_new_from_string(
            self->m_xkb_ctx,
            map_shm,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS
        );

        munmap(map_shm, size);
        ::close(fd);

        if (self->m_xkb_state) xkb_state_unref(self->m_xkb_state);
        self->m_xkb_state = xkb_state_new(self->m_xkb_keymap);
    },
    .enter = [](void* data, struct wl_keyboard*, uint32_t, struct wl_surface*, struct wl_array*) {
        auto* self = static_cast<Window*>(data);
        self->m_has_keyboard_focus = true;
    },
    .leave = [](void* data, struct wl_keyboard*, uint32_t, struct wl_surface*) {
        auto* self = static_cast<Window*>(data);
        if (self->m_has_keyboard_focus) {
            self->m_has_keyboard_focus = false;
            if (self->m_close_on_click_outside) {
                if (self->m_on_close) self->m_on_close();
                self->close();
            }
        }
    },
    .key = [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
        auto* self = static_cast<Window*>(data);
        if (!self->m_xkb_state) return;

        uint32_t keycode = key + 8;
        xkb_keysym_t sym = xkb_state_key_get_one_sym(self->m_xkb_state, keycode);
        bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

        char utf8_buf[64] = {0};
        xkb_state_key_get_utf8(self->m_xkb_state, keycode, utf8_buf, sizeof(utf8_buf));

        KeyPressEvent event;
        event.keysym = sym;
        event.raw_code = keycode;
        event.utf8_text = utf8_buf;
        event.pressed = pressed;
        event.modifiers = self->m_modifiers;

        if (self->m_popup_view) {
            if (self->m_popup_view->on_key(event)) {
                self->schedule_redraw();
                return;
            }
            if (pressed && sym == XKB_KEY_Escape) {
                if (!self->m_active_popup || self->m_active_popup->is_dismiss_on_escape()) {
                    self->dismiss_popup();
                    return;
                }
            }
        }

        if (pressed && self->m_close_on_escape && sym == XKB_KEY_Escape) {
            if (self->m_on_close) self->m_on_close();
            self->close();
            return;
        }

        if (self->m_on_key) {
            self->m_on_key(event);
        }

        if (self->m_focused_view) {
            if (self->m_focused_view->on_key(event)) {
                self->schedule_redraw();
                return;
            }
        }

        if (self->m_root_view) {
            if (self->m_root_view->on_key(event)) {
                self->schedule_redraw();
            }
        }
    },
    .modifiers = [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
        auto* self = static_cast<Window*>(data);
        if (self->m_xkb_state) {
            xkb_state_update_mask(self->m_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
            self->m_modifiers = 0;
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Shift);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_CAPS, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Caps);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Control);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Alt);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Super);
        }
    },
    .repeat_info = [](void*, struct wl_keyboard*, int32_t, int32_t) {}
};

const struct wl_touch_listener Window::s_touch_listener = {
    .down = [](void* data, struct wl_touch*, uint32_t serial, uint32_t time, struct wl_surface*, int32_t id, wl_fixed_t x, wl_fixed_t y) {
        auto* self = static_cast<Window*>(data);
        double tx = wl_fixed_to_double(x);
        double ty = wl_fixed_to_double(y);

        self->m_primary_touch_id = id;
        self->m_touch_active = true;
        self->m_last_x = tx;
        self->m_last_y = ty;

        TouchEvent event{ id, tx, ty, TouchPhase::Down, time };

        // 1. Popup overlay outside dismissal
        if (self->m_popup_view) {
            if (!self->m_popup_bounds.contains(tx, ty)) {
                if (!self->m_active_popup || self->m_active_popup->is_dismiss_on_outside_click()) {
                    self->dismiss_popup();
                    return;
                }
            }
            if (self->m_popup_view->on_touch(event, self->m_popup_bounds)) {
                self->m_touch_consumed_by_view = true;
                self->schedule_redraw();
                return;
            }
            // Mouse fallback
            self->m_touch_consumed_by_view = false;
            self->m_popup_view->on_mouse_move(tx, ty, self->m_popup_bounds);
            if (self->m_popup_view->on_mouse_button(tx, ty, MouseButton::Left, true, self->m_popup_bounds)) {
                self->schedule_redraw();
            }
            return;
        }

        // 2. Close on click outside
        if (self->m_close_on_click_outside && !self->m_allocated_content_bounds.contains(tx, ty)) {
            if (self->m_on_close) self->m_on_close();
            self->close();
            return;
        }

        // 3. Root View
        if (self->m_root_view) {
            View* prev_focused = self->m_focused_view;
            if (self->m_root_view->on_touch(event, self->m_allocated_content_bounds)) {
                self->m_touch_consumed_by_view = true;
                if (prev_focused && self->m_focused_view == prev_focused) {
                    if (!prev_focused->contains_point(tx, ty)) {
                        self->clear_focus();
                    }
                }
                self->schedule_redraw();
                return;
            }
            // Fallback mouse emulation
            self->m_touch_consumed_by_view = false;
            self->m_root_view->on_mouse_move(tx, ty, self->m_allocated_content_bounds);
            if (self->m_root_view->on_mouse_button(tx, ty, MouseButton::Left, true, self->m_allocated_content_bounds)) {
                self->schedule_redraw();
            }

            if (prev_focused && self->m_focused_view == prev_focused) {
                if (!prev_focused->contains_point(tx, ty)) {
                    self->clear_focus();
                }
            }
        }
    },
    .up = [](void* data, struct wl_touch*, uint32_t serial, uint32_t time, int32_t id) {
        auto* self = static_cast<Window*>(data);
        TouchEvent event{ id, self->m_last_x, self->m_last_y, TouchPhase::Up, time };

        if (self->m_popup_view) {
            if (self->m_touch_consumed_by_view) {
                if (self->m_popup_view->on_touch(event, self->m_popup_bounds)) {
                    self->schedule_redraw();
                }
            } else {
                if (self->m_popup_view->on_mouse_button(self->m_last_x, self->m_last_y, MouseButton::Left, false, self->m_popup_bounds)) {
                    self->schedule_redraw();
                }
            }
        } else if (self->m_root_view) {
            if (self->m_touch_consumed_by_view) {
                if (self->m_root_view->on_touch(event, self->m_allocated_content_bounds)) {
                    self->schedule_redraw();
                }
            } else {
                if (self->m_root_view->on_mouse_button(self->m_last_x, self->m_last_y, MouseButton::Left, false, self->m_allocated_content_bounds)) {
                    self->schedule_redraw();
                }
            }
        }

        self->m_primary_touch_id = -1;
        self->m_touch_active = false;
        self->m_touch_consumed_by_view = false;
    },
    .motion = [](void* data, struct wl_touch*, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y) {
        auto* self = static_cast<Window*>(data);
        double tx = wl_fixed_to_double(x);
        double ty = wl_fixed_to_double(y);

        self->m_last_x = tx;
        self->m_last_y = ty;

        TouchEvent event{ id, tx, ty, TouchPhase::Motion, time };

        if (self->m_popup_view) {
            if (self->m_touch_consumed_by_view) {
                if (self->m_popup_view->on_touch(event, self->m_popup_bounds)) {
                    self->schedule_redraw();
                }
            } else {
                if (self->m_popup_view->on_mouse_move(tx, ty, self->m_popup_bounds)) {
                    self->schedule_redraw();
                }
            }
        } else if (self->m_root_view) {
            if (self->m_touch_consumed_by_view) {
                if (self->m_root_view->on_touch(event, self->m_allocated_content_bounds)) {
                    self->schedule_redraw();
                }
            } else {
                if (self->m_root_view->on_mouse_move(tx, ty, self->m_allocated_content_bounds)) {
                    self->schedule_redraw();
                }
            }
        }
    },
    .frame = [](void*, struct wl_touch*) {},
    .cancel = [](void* data, struct wl_touch*) {
        auto* self = static_cast<Window*>(data);
        TouchEvent event{ self->m_primary_touch_id, self->m_last_x, self->m_last_y, TouchPhase::Cancel, 0 };

        if (self->m_popup_view) {
            if (self->m_touch_consumed_by_view) {
                self->m_popup_view->on_touch(event, self->m_popup_bounds);
            } else {
                self->m_popup_view->on_mouse_button(self->m_last_x, self->m_last_y, MouseButton::Left, false, self->m_popup_bounds);
            }
        } else if (self->m_root_view) {
            if (self->m_touch_consumed_by_view) {
                self->m_root_view->on_touch(event, self->m_allocated_content_bounds);
            } else {
                self->m_root_view->on_mouse_button(self->m_last_x, self->m_last_y, MouseButton::Left, false, self->m_allocated_content_bounds);
            }
        }

        self->m_primary_touch_id = -1;
        self->m_touch_active = false;
        self->m_touch_consumed_by_view = false;
        self->schedule_redraw();
    },
#ifdef WL_TOUCH_SHAPE_SINCE_VERSION
    .shape = [](void*, struct wl_touch*, int32_t, wl_fixed_t, wl_fixed_t) {},
#endif
#ifdef WL_TOUCH_ORIENTATION_SINCE_VERSION
    .orientation = [](void*, struct wl_touch*, int32_t, wl_fixed_t) {},
#endif
};

const struct wl_callback_listener Window::s_frame_listener = {
    .done = [](void* data, struct wl_callback* callback, uint32_t time) {
        auto* self = static_cast<Window*>(data);
        if (self->m_frame_callback) {
            wl_callback_destroy(self->m_frame_callback);
            self->m_frame_callback = nullptr;
        }
        if (self->m_needs_redraw) {
            self->render_frame();
        }
    }
};

Window::Window() {
    m_anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
}

Window::~Window() {
    if (m_cursor_surface) wl_surface_destroy(m_cursor_surface);
    if (m_cursor_theme) wl_cursor_theme_destroy(m_cursor_theme);
    if (m_frame_callback) wl_callback_destroy(m_frame_callback);
    if (m_xkb_state) xkb_state_unref(m_xkb_state);
    if (m_xkb_keymap) xkb_keymap_unref(m_xkb_keymap);
    if (m_xkb_ctx) xkb_context_unref(m_xkb_ctx);
    if (m_pointer) wl_pointer_destroy(m_pointer);
    if (m_keyboard) wl_keyboard_destroy(m_keyboard);
    if (m_touch) wl_touch_destroy(m_touch);
    if (m_xdg_toplevel) xdg_toplevel_destroy(m_xdg_toplevel);
    if (m_xdg_surface) xdg_surface_destroy(m_xdg_surface);
    if (m_layer_surface) zwlr_layer_surface_v1_destroy(m_layer_surface);
    if (m_session_lock_surface) ext_session_lock_surface_v1_destroy(m_session_lock_surface);
    if (m_surface) wl_surface_destroy(m_surface);
}


bool Window::init() {
    auto* engine = AppEngine::instance();
    if (!engine) return false;

    m_surface = wl_compositor_create_surface(engine->get_compositor());
    if (!m_surface) return false;

    if (m_role == WindowRole::SessionLock) {
        auto* lock = engine->get_session_lock();
        if (!lock) {
            std::cerr << "[miqutoolkit] SessionLock window requested without active session lock." << std::endl;
            return false;
        }

        m_session_lock_surface = ext_session_lock_v1_get_lock_surface(lock, m_surface, m_output);
        if (!m_session_lock_surface) {
            std::cerr << "[miqutoolkit] Failed to create ext_session_lock_surface_v1." << std::endl;
            return false;
        }

        ext_session_lock_surface_v1_add_listener(m_session_lock_surface, &s_lock_surface_listener, this);

        if (engine->get_seat() && engine->get_seat_capabilities() != 0) {
            update_seat_capabilities(engine->get_seat_capabilities());
        }

        wl_display_roundtrip(engine->get_display());

        engine->register_window(shared_from_this());
        return true;
    }

    if (m_role == WindowRole::Toplevel) {
        if (!engine->get_xdg_wm_base()) {
            std::cerr << "[miqutoolkit] xdg_wm_base protocol not available for Toplevel window." << std::endl;
            return false;
        }

        m_xdg_surface = xdg_wm_base_get_xdg_surface(engine->get_xdg_wm_base(), m_surface);
        if (!m_xdg_surface) return false;
        xdg_surface_add_listener(m_xdg_surface, &s_xdg_surface_listener, this);

        m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
        if (!m_xdg_toplevel) return false;
        xdg_toplevel_add_listener(m_xdg_toplevel, &s_xdg_toplevel_listener, this);

        xdg_toplevel_set_title(m_xdg_toplevel, m_title.c_str());
        xdg_toplevel_set_app_id(m_xdg_toplevel, m_app_id.c_str());

        if (engine->get_seat() && engine->get_seat_capabilities() != 0) {
            update_seat_capabilities(engine->get_seat_capabilities());
        }

        wl_surface_commit(m_surface);
        wl_display_roundtrip(engine->get_display());

        engine->register_window(shared_from_this());
        return true;
    }

    if (!engine->get_layer_shell()) {
        std::cerr << "[miqutoolkit] zwlr_layer_shell_v1 protocol not available for Layer window." << std::endl;
        return false;
    }

    uint32_t layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    if (m_role == WindowRole::LayerTop) layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    else if (m_role == WindowRole::LayerBottom) layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
    else if (m_role == WindowRole::LayerBackground) layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;

    std::string ns = m_layer_namespace;
    if (ns.empty()) {
        ns = m_app_id.empty() ? "miqutoolkit-layer" : m_app_id;
    }

    m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        engine->get_layer_shell(),
        m_surface,
        m_output,
        layer,
        ns.c_str()
    );

    if (!m_layer_surface) return false;

    zwlr_layer_surface_v1_add_listener(m_layer_surface, &s_layer_surface_listener, this);

    if (engine->get_seat() && engine->get_seat_capabilities() != 0) {
        update_seat_capabilities(engine->get_seat_capabilities());
    }

    if (m_content_w > 0 && m_content_h > 0 && m_anchors == 0) {
        m_width = m_content_w;
        m_height = m_content_h;
    }

    bool stretch_x = (m_anchors & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) &&
                     (m_anchors & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    bool stretch_y = (m_anchors & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) &&
                     (m_anchors & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);

    uint32_t req_w = stretch_x ? 0 : static_cast<uint32_t>(std::max(0, m_width));
    uint32_t req_h = stretch_y ? 0 : static_cast<uint32_t>(std::max(0, m_height));

    zwlr_layer_surface_v1_set_size(m_layer_surface, req_w, req_h);
    zwlr_layer_surface_v1_set_anchor(m_layer_surface, m_anchors);
    zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, m_exclusive_zone);

    if (m_kb_interactive) {
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    }

    wl_surface_commit(m_surface);
    wl_display_roundtrip(engine->get_display());

    engine->register_window(shared_from_this());
    return true;
}

void Window::show() {
    schedule_redraw();
}

void Window::set_content_view(std::shared_ptr<View> view) {
    m_root_view = view;
    if (m_root_view) {
        m_root_view->set_window(this);
    }
    schedule_redraw();
}

void Window::schedule_redraw() {
    if (!m_configured) return;
    if (m_rendering || m_frame_callback) {
        m_needs_redraw = true;
        return;
    }
    render_frame();
}

void Window::refresh_theme() {
    schedule_redraw();
}

void Window::render_frame() {
    if (!m_configured || !m_shm_pool || !m_surface) return;
    m_needs_redraw = false;
    m_rendering = true;

    auto* buf = m_shm_pool->get_next_buffer();
    if (!buf || !buf->cr) {
        m_rendering = false;
        return;
    }

    // Clear buffer
    cairo_save(buf->cr);
    cairo_set_operator(buf->cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(buf->cr);
    cairo_restore(buf->cr);

    if (m_role == WindowRole::Toplevel) {
        if (!m_transparent) {
            Color bg = m_has_custom_bg ? m_background_color : Config::get()->colors.background;
            if (bg.a > 0.0f) {
                cairo_save(buf->cr);
                cairo_set_source_rgba(buf->cr, bg.r, bg.g, bg.b, bg.a);
                cairo_rectangle(buf->cr, 0, 0, m_width, m_height);
                cairo_fill(buf->cr);
                cairo_restore(buf->cr);
            }
        }

        if (m_root_view && m_root_view->is_visible()) {
            Rect content_bounds(0, 0, m_width, m_height);
            m_allocated_content_bounds = content_bounds;
            m_root_view->draw(buf->cr, content_bounds);
        }
    } else {
        // 1. Draw Backdrop / Background if not transparent
        if (!m_transparent) {
            if (m_has_custom_bg && m_background_color.a > 0.0f) {
                cairo_save(buf->cr);
                cairo_set_source_rgba(buf->cr, m_background_color.r,
                                              m_background_color.g,
                                              m_background_color.b,
                                              m_background_color.a);
                cairo_rectangle(buf->cr, 0, 0, m_width, m_height);
                cairo_fill(buf->cr);
                cairo_restore(buf->cr);
            } else if (m_dim_backdrop) {
                auto config = Config::get();
                cairo_save(buf->cr);
                cairo_set_source_rgba(buf->cr, config->colors.backdrop.r,
                                              config->colors.backdrop.g,
                                              config->colors.backdrop.b,
                                              config->colors.backdrop.a);
                cairo_rectangle(buf->cr, 0, 0, m_width, m_height);
                cairo_fill(buf->cr);
                cairo_restore(buf->cr);
            }
        }

        // 2. Draw Content View
        if (m_root_view && m_root_view->is_visible()) {
            Rect content_bounds(0, 0, m_width, m_height);
            if (m_content_w > 0 && m_content_h > 0) {
                int cw = std::min(m_content_w, m_width);
                int ch = std::min(m_content_h, m_height);
                int cx = (m_width - cw) / 2;
                int cy = (m_height - ch) / 2;
                content_bounds = Rect(cx, cy, cw, ch);
            }
            m_allocated_content_bounds = content_bounds;
            m_root_view->draw(buf->cr, content_bounds);
        }
    }

    // 3. Draw Active Popup Overlay on top of scene
    if (m_popup_view && m_popup_view->is_visible()) {
        m_popup_view->draw(buf->cr, m_popup_bounds);
    }

    cairo_surface_flush(buf->cairo_surf);
    buf->busy = true;

    wl_surface_attach(m_surface, buf->wl_buf, 0, 0);
    wl_surface_damage_buffer(m_surface, 0, 0, m_width, m_height);

    m_frame_callback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_frame_callback, &s_frame_listener, this);

    wl_surface_commit(m_surface);
    wl_display_flush(AppEngine::instance()->get_display());
    m_rendering = false;
}

void Window::show_popup(std::shared_ptr<PopupWindow> popup) {
    if (m_active_popup && m_active_popup != popup) {
        auto old_popup = m_active_popup;
        m_active_popup = nullptr;
        m_popup_view = nullptr;
        old_popup->notify_dismissed();
    }
    m_active_popup = popup;
    if (m_active_popup) {
        m_popup_view = m_active_popup->get_container_view();
        m_popup_bounds = m_active_popup->get_bounds();
        if (m_popup_view) {
            m_popup_view->set_window(this);
            m_popup_view->set_bounds(m_popup_bounds);
        }
    } else {
        m_popup_view = nullptr;
    }
    schedule_redraw();
}

void Window::show_popup(std::shared_ptr<View> popup, const Rect& bounds) {
    if (m_active_popup) {
        auto old_popup = m_active_popup;
        m_active_popup = nullptr;
        m_popup_view = nullptr;
        old_popup->notify_dismissed();
    }
    m_popup_view = popup;
    m_popup_bounds = bounds;
    if (m_popup_view) {
        m_popup_view->set_window(this);
        m_popup_view->set_bounds(bounds);
    }
    schedule_redraw();
}

void Window::dismiss_popup() {
    auto old_popup = m_active_popup;
    auto old_view = m_popup_view;
    m_active_popup = nullptr;
    m_popup_view = nullptr;

    if (old_popup) {
        old_popup->notify_dismissed();
    }
    if (old_view) {
        schedule_redraw();
    }
}

void Window::close() {
    dismiss_popup();
    auto* engine = AppEngine::instance();
    if (engine) {
        engine->unregister_window(shared_from_this());
    }
    if (m_xdg_toplevel) {
        xdg_toplevel_destroy(m_xdg_toplevel);
        m_xdg_toplevel = nullptr;
    }
    if (m_xdg_surface) {
        xdg_surface_destroy(m_xdg_surface);
        m_xdg_surface = nullptr;
    }
    if (m_layer_surface) {
        zwlr_layer_surface_v1_destroy(m_layer_surface);
        m_layer_surface = nullptr;
    }
    if (m_session_lock_surface) {
        ext_session_lock_surface_v1_destroy(m_session_lock_surface);
        m_session_lock_surface = nullptr;
    }
    if (m_frame_callback) {
        wl_callback_destroy(m_frame_callback);
        m_frame_callback = nullptr;
    }
    if (m_surface) {
        wl_surface_destroy(m_surface);
        m_surface = nullptr;
    }
    m_configured = false;
    if (engine && engine->get_display()) {
        wl_display_flush(engine->get_display());
    }
}

void Window::request_close() {
    if (m_role == WindowRole::Toplevel) {
        auto* engine = AppEngine::instance();
        if (engine) {
            engine->get_foreign_toplevel_manager();
            auto* wm = WindowManager::get();
            if (wm && wm->is_supported()) {
                // 1. Prioritize active window if it matches our app_id
                const auto* active = wm->get_active_window();
                if (active && (m_app_id.empty() || active->app_id == m_app_id)) {
                    const_cast<WindowInfo*>(active)->close();
                    return;
                }

                auto windows = wm->get_windows();
                // 2. Match by both title and app_id
                if (!m_title.empty()) {
                    for (auto& win : windows) {
                        if (win.title == m_title && (m_app_id.empty() || win.app_id == m_app_id)) {
                            win.close();
                            return;
                        }
                    }
                }

                // 3. Fallback: match by app_id
                for (auto& win : windows) {
                    if (!m_app_id.empty() && win.app_id == m_app_id) {
                        win.close();
                        return;
                    }
                }

                // 4. Fallback: any active window
                if (active) {
                    const_cast<WindowInfo*>(active)->close();
                    return;
                }
            }
        }
    }

    if (m_on_close) {
        m_on_close();
    }
    close();
}

void Window::update_cursor(uint32_t serial) {
    if (!m_pointer) return;
    auto* engine = AppEngine::instance();
    if (!engine || !engine->get_shm() || !engine->get_compositor()) return;

    if (!m_cursor_theme) {
        const char* xcursor_theme = getenv("XCURSOR_THEME");
        const char* xcursor_size = getenv("XCURSOR_SIZE");
        int size = 24;
        if (xcursor_size && *xcursor_size) {
            try { size = std::stoi(xcursor_size); } catch (...) {}
        }
        m_cursor_theme = wl_cursor_theme_load(xcursor_theme, size, engine->get_shm());
    }
    if (!m_cursor_theme) return;

    struct wl_cursor* cursor = wl_cursor_theme_get_cursor(m_cursor_theme, "default");
    if (!cursor) cursor = wl_cursor_theme_get_cursor(m_cursor_theme, "left_ptr");
    if (!cursor || cursor->image_count == 0) return;

    struct wl_cursor_image* image = cursor->images[0];
    struct wl_buffer* buffer = wl_cursor_image_get_buffer(image);
    if (!buffer) return;

    if (!m_cursor_surface) {
        m_cursor_surface = wl_compositor_create_surface(engine->get_compositor());
    }
    if (!m_cursor_surface) return;

    wl_surface_attach(m_cursor_surface, buffer, 0, 0);
    wl_surface_damage(m_cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(m_cursor_surface);

    wl_pointer_set_cursor(m_pointer, serial, m_cursor_surface, image->hotspot_x, image->hotspot_y);
}

void Window::set_focused_view(View* view) {
    if (m_focused_view == view) return;
    View* old_focus = m_focused_view;
    m_focused_view = view;
    if (old_focus) {
        old_focus->on_focus_changed(false);
    }
    if (m_focused_view) {
        m_focused_view->on_focus_changed(true);
    }
    schedule_redraw();
}

void Window::clear_focus() {
    set_focused_view(nullptr);
}

} // namespace miqu
