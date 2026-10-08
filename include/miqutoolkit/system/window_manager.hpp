#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

struct zwlr_foreign_toplevel_handle_v1;
struct zwlr_foreign_toplevel_manager_v1;
struct zwlr_foreign_toplevel_handle_v1_listener;
struct zwlr_foreign_toplevel_manager_v1_listener;

namespace miqu {

class WindowManager;

struct WindowInfo {
    uint64_t id = 0;
    std::string title;
    std::string app_id;
    bool is_active = false;
    bool is_minimized = false;
    bool is_maximized = false;
    bool is_fullscreen = false;

    void activate();
    void close();
    void set_minimized(bool minimized);
    void set_maximized(bool maximized);

    struct zwlr_foreign_toplevel_handle_v1* get_handle() const { return handle; }

private:
    friend class WindowManager;
    struct zwlr_foreign_toplevel_handle_v1* handle = nullptr;
    WindowManager* manager = nullptr;
};

class WindowManager {
public:
    static WindowManager* get();

    std::vector<WindowInfo> get_windows() const;
    const WindowInfo* get_active_window() const;

    void on_windows_changed(std::function<void()> callback);

    bool is_supported() const { return m_manager != nullptr; }

    void init_protocol(struct zwlr_foreign_toplevel_manager_v1* mgr);
    void clear();

private:
    friend struct WindowInfo;
    WindowManager() = default;
    ~WindowManager();

    void notify_changed();

    struct zwlr_foreign_toplevel_manager_v1* m_manager = nullptr;
    std::vector<std::shared_ptr<WindowInfo>> m_windows;
    std::vector<std::function<void()>> m_listeners;

    static const struct ::zwlr_foreign_toplevel_manager_v1_listener s_manager_listener;
    static const struct ::zwlr_foreign_toplevel_handle_v1_listener s_handle_listener;
};

} // namespace miqu
