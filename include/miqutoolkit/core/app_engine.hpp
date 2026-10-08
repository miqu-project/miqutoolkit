#pragma once

#include <wayland-client.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <mutex>

struct zwlr_layer_shell_v1;
struct zwlr_foreign_toplevel_manager_v1;
struct ext_workspace_manager_v1;
struct ext_idle_notifier_v1;
struct ext_session_lock_manager_v1;
struct ext_session_lock_v1;
struct ext_session_lock_v1_listener;
struct ext_session_lock_surface_v1;
struct xdg_wm_base;
struct xdg_wm_base_listener;

namespace miqu {

class Window;

class AppEngine {
public:
    static std::shared_ptr<AppEngine> create();
    ~AppEngine();

    int enter_loop();
    void quit(int exit_code = 0);

    void post(std::function<void()> task);
    void request_redraw_all();

    struct wl_display* get_display() const { return m_display; }
    struct wl_compositor* get_compositor() const { return m_compositor; }
    struct wl_shm* get_shm() const { return m_shm; }
    struct zwlr_layer_shell_v1* get_layer_shell() const { return m_layer_shell; }
    struct zwlr_foreign_toplevel_manager_v1* get_foreign_toplevel_manager();
    struct ext_workspace_manager_v1* get_workspace_manager_protocol();
    struct ext_idle_notifier_v1* get_idle_notifier() const { return m_idle_notifier; }
    struct ext_session_lock_manager_v1* get_session_lock_manager() const { return m_session_lock_manager; }
    struct ext_session_lock_v1* get_session_lock() const { return m_session_lock; }
    struct xdg_wm_base* get_xdg_wm_base() const { return m_xdg_wm_base; }
    struct wl_seat* get_seat() const { return m_seat; }
    uint32_t get_seat_capabilities() const { return m_seat_capabilities; }

    bool lock_session(std::function<void(bool success)> on_locked);
    void unlock_session();
    bool is_session_locked() const { return m_session_lock != nullptr; }

    void register_window(std::shared_ptr<Window> win);
    void unregister_window(std::shared_ptr<Window> win);

    void set_quit_on_last_window_closed(bool quit) { m_quit_on_last_window = quit; }
    bool get_quit_on_last_window_closed() const { return m_quit_on_last_window; }

    void add_theme_change_listener(std::function<void()> listener) { m_theme_change_listeners.push_back(std::move(listener)); }

    static AppEngine* instance() { return s_instance; }

    void setup_config_watcher();

private:
    AppEngine() = default;
    bool init();

    static void registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name);
    static const struct wl_registry_listener s_registry_listener;
    static const struct wl_seat_listener s_seat_listener;
    static const struct ::xdg_wm_base_listener s_wm_base_listener;
    static const struct ::ext_session_lock_v1_listener s_session_lock_listener;

    static AppEngine* s_instance;

    struct wl_display* m_display = nullptr;
    struct wl_registry* m_registry = nullptr;
    struct wl_compositor* m_compositor = nullptr;
    struct wl_shm* m_shm = nullptr;
    struct zwlr_layer_shell_v1* m_layer_shell = nullptr;
    struct zwlr_foreign_toplevel_manager_v1* m_foreign_toplevel_manager = nullptr;
    uint32_t m_foreign_toplevel_manager_name = 0;
    uint32_t m_foreign_toplevel_manager_version = 0;
    struct ext_workspace_manager_v1* m_ext_workspace_manager = nullptr;
    uint32_t m_ext_workspace_manager_name = 0;
    uint32_t m_ext_workspace_manager_version = 0;
    struct ext_idle_notifier_v1* m_idle_notifier = nullptr;
    struct ext_session_lock_manager_v1* m_session_lock_manager = nullptr;
    struct ext_session_lock_v1* m_session_lock = nullptr;
    std::function<void(bool success)> m_on_locked_cb;
    struct xdg_wm_base* m_xdg_wm_base = nullptr;
    struct wl_seat* m_seat = nullptr;
    uint32_t m_seat_capabilities = 0;

    bool m_running = false;
    bool m_quit_on_last_window = true;
    int m_exit_code = 0;
    std::vector<std::shared_ptr<Window>> m_windows;

    int m_wakeup_fd = -1;
    std::mutex m_tasks_mutex;
    std::vector<std::function<void()>> m_posted_tasks;

    int m_inotify_fd = -1;
    std::vector<std::pair<int, std::string>> m_inotify_watches;
    std::vector<std::function<void()>> m_theme_change_listeners;

    void handle_inotify_events();
};

} // namespace miqu
