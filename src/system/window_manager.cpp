#include "miqutoolkit/system/window_manager.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include <algorithm>
#include <iostream>

namespace miqu {

WindowManager* WindowManager::get() {
    static WindowManager s_instance;
    auto* engine = AppEngine::instance();
    if (engine && !s_instance.m_manager) {
        engine->get_foreign_toplevel_manager();
    }
    return &s_instance;
}

void WindowManager::clear() {
    for (auto& win : m_windows) {
        if (win && win->handle) {
            zwlr_foreign_toplevel_handle_v1_destroy(win->handle);
            win->handle = nullptr;
        }
    }
    m_windows.clear();
    m_listeners.clear();
    m_manager = nullptr;
}

WindowManager::~WindowManager() {
    clear();
}

void WindowInfo::activate() {
    if (!handle) return;
    auto* engine = AppEngine::instance();
    if (engine && engine->get_seat()) {
        zwlr_foreign_toplevel_handle_v1_activate(handle, engine->get_seat());
        wl_display_flush(engine->get_display());
    }
}

void WindowInfo::close() {
    if (!handle) return;
    zwlr_foreign_toplevel_handle_v1_close(handle);
    auto* engine = AppEngine::instance();
    if (engine) {
        wl_display_flush(engine->get_display());
    }
}

void WindowInfo::set_minimized(bool min) {
    if (!handle) return;
    if (min) {
        zwlr_foreign_toplevel_handle_v1_set_minimized(handle);
    } else {
        zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
    }
    auto* engine = AppEngine::instance();
    if (engine) {
        wl_display_flush(engine->get_display());
    }
}

void WindowInfo::set_maximized(bool max) {
    if (!handle) return;
    if (max) {
        zwlr_foreign_toplevel_handle_v1_set_maximized(handle);
    } else {
        zwlr_foreign_toplevel_handle_v1_unset_maximized(handle);
    }
    auto* engine = AppEngine::instance();
    if (engine) {
        wl_display_flush(engine->get_display());
    }
}

const struct ::zwlr_foreign_toplevel_handle_v1_listener WindowManager::s_handle_listener = {
    .title = [](void* data, struct zwlr_foreign_toplevel_handle_v1*, const char* title) {
        auto* win = static_cast<WindowInfo*>(data);
        win->title = title ? title : "";
    },
    .app_id = [](void* data, struct zwlr_foreign_toplevel_handle_v1*, const char* app_id) {
        auto* win = static_cast<WindowInfo*>(data);
        win->app_id = app_id ? app_id : "";
    },
    .output_enter = [](void*, struct zwlr_foreign_toplevel_handle_v1*, struct wl_output*) {},
    .output_leave = [](void*, struct zwlr_foreign_toplevel_handle_v1*, struct wl_output*) {},
    .state = [](void* data, struct zwlr_foreign_toplevel_handle_v1*, struct wl_array* state) {
        auto* win = static_cast<WindowInfo*>(data);
        win->is_active = false;
        win->is_minimized = false;
        win->is_maximized = false;
        win->is_fullscreen = false;

        if (state && state->data) {
            auto* entries = static_cast<const uint32_t*>(state->data);
            size_t count = state->size / sizeof(uint32_t);
            for (size_t i = 0; i < count; ++i) {
                uint32_t val = entries[i];
                if (val == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
                    win->is_active = true;
                } else if (val == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED) {
                    win->is_minimized = true;
                } else if (val == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED) {
                    win->is_maximized = true;
                } else if (val == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN) {
                    win->is_fullscreen = true;
                }
            }
        }
    },
    .done = [](void* data, struct zwlr_foreign_toplevel_handle_v1*) {
        auto* win = static_cast<WindowInfo*>(data);
        if (win && win->manager) {
            win->manager->notify_changed();
        }
    },
    .closed = [](void* data, struct zwlr_foreign_toplevel_handle_v1* handle) {
        auto* win = static_cast<WindowInfo*>(data);
        WindowManager* mgr = win ? win->manager : nullptr;
        zwlr_foreign_toplevel_handle_v1_destroy(handle);
        if (mgr) {
            auto& wins = mgr->m_windows;
            wins.erase(std::remove_if(wins.begin(), wins.end(),
                [win](const std::shared_ptr<WindowInfo>& w) { return w.get() == win; }),
                wins.end());
            mgr->notify_changed();
        }
    },
    .parent = [](void*, struct zwlr_foreign_toplevel_handle_v1*, struct zwlr_foreign_toplevel_handle_v1*) {}
};

const struct ::zwlr_foreign_toplevel_manager_v1_listener WindowManager::s_manager_listener = {
    .toplevel = [](void* data, struct zwlr_foreign_toplevel_manager_v1*, struct zwlr_foreign_toplevel_handle_v1* handle) {
        auto* self = static_cast<WindowManager*>(data);
        auto win = std::make_shared<WindowInfo>();
        win->handle = handle;
        win->manager = self;
        win->id = reinterpret_cast<uintptr_t>(handle);
        self->m_windows.push_back(win);
        zwlr_foreign_toplevel_handle_v1_add_listener(handle, &s_handle_listener, win.get());
    },
    .finished = [](void* data, struct zwlr_foreign_toplevel_manager_v1*) {
        auto* self = static_cast<WindowManager*>(data);
        self->m_manager = nullptr;
    }
};

void WindowManager::init_protocol(struct zwlr_foreign_toplevel_manager_v1* mgr) {
    if (m_manager == mgr) return;
    m_manager = mgr;
    if (m_manager) {
        zwlr_foreign_toplevel_manager_v1_add_listener(m_manager, &s_manager_listener, this);
    }
}

std::vector<WindowInfo> WindowManager::get_windows() const {
    std::vector<WindowInfo> result;
    result.reserve(m_windows.size());
    for (const auto& w : m_windows) {
        if (w) {
            result.push_back(*w);
        }
    }
    return result;
}

const WindowInfo* WindowManager::get_active_window() const {
    for (const auto& w : m_windows) {
        if (w && w->is_active) {
            return w.get();
        }
    }
    return nullptr;
}

void WindowManager::on_windows_changed(std::function<void()> callback) {
    if (callback) {
        m_listeners.push_back(std::move(callback));
    }
}

void WindowManager::notify_changed() {
    for (const auto& cb : m_listeners) {
        if (cb) cb();
    }
}

} // namespace miqu
