#include "miqutoolkit/core/app_engine.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/fs_utils.hpp"
#include "miqutoolkit/system/window_manager.hpp"
#include "miqutoolkit/system/workspace_manager.hpp"
#include "miqutoolkit/system/output_manager.hpp"
#include "miqutoolkit/system/idle_manager.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <map>
#include <filesystem>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "ext-workspace-v1-client-protocol.h"
#include "ext-idle-notify-v1-client-protocol.h"
#include "ext-session-lock-v1-client-protocol.h"

namespace miqu {

AppEngine* AppEngine::s_instance = nullptr;

const struct ext_session_lock_v1_listener AppEngine::s_session_lock_listener = {
    .locked = [](void* data, struct ext_session_lock_v1*) {
        auto* self = static_cast<AppEngine*>(data);
        if (self->m_on_locked_cb) {
            auto cb = std::move(self->m_on_locked_cb);
            cb(true);
        }
    },
    .finished = [](void* data, struct ext_session_lock_v1* lock) {
        auto* self = static_cast<AppEngine*>(data);
        if (self->m_on_locked_cb) {
            auto cb = std::move(self->m_on_locked_cb);
            cb(false);
        }
        if (self->m_session_lock == lock) {
            ext_session_lock_v1_destroy(self->m_session_lock);
            self->m_session_lock = nullptr;
        }
    }
};

const struct xdg_wm_base_listener AppEngine::s_wm_base_listener = {
    .ping = [](void*, struct xdg_wm_base* wm_base, uint32_t serial) {
        xdg_wm_base_pong(wm_base, serial);
    }
};

const struct wl_registry_listener AppEngine::s_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

const struct wl_seat_listener AppEngine::s_seat_listener = {
    .capabilities = [](void* data, struct wl_seat*, uint32_t caps) {
        auto* self = static_cast<AppEngine*>(data);
        self->m_seat_capabilities = caps;
        for (auto& win : self->m_windows) {
            if (win) win->update_seat_capabilities(caps);
        }
    },
    .name = [](void*, struct wl_seat*, const char*) {}
};

std::shared_ptr<AppEngine> AppEngine::create() {
    if (s_instance) {
        return nullptr;
    }

    auto engine = std::shared_ptr<AppEngine>(new AppEngine());
    if (!engine->init()) {
        return nullptr;
    }

    s_instance = engine.get();
    return engine;
}

AppEngine::~AppEngine() {
    m_windows.clear();

    if (m_wakeup_fd >= 0) {
        close(m_wakeup_fd);
        m_wakeup_fd = -1;
    }

    for (const auto& [wd, fname] : m_inotify_watches) {
        if (wd >= 0 && m_inotify_fd >= 0) {
            inotify_rm_watch(m_inotify_fd, wd);
        }
    }
    m_inotify_watches.clear();

    if (m_inotify_fd >= 0) {
        close(m_inotify_fd);
        m_inotify_fd = -1;
    }

    if (m_session_lock) {
        unlock_session();
    }
    IdleManager::get()->clear_listeners();
    OutputManager::get()->clear();
    WindowManager::get()->clear();
    WorkspaceManager::get()->clear();
    ImageView::clear_cache();
    if (m_session_lock_manager) ext_session_lock_manager_v1_destroy(m_session_lock_manager);
    if (m_idle_notifier) ext_idle_notifier_v1_destroy(m_idle_notifier);
    if (m_ext_workspace_manager) ext_workspace_manager_v1_destroy(m_ext_workspace_manager);
    if (m_foreign_toplevel_manager) zwlr_foreign_toplevel_manager_v1_destroy(m_foreign_toplevel_manager);
    if (m_xdg_wm_base) xdg_wm_base_destroy(m_xdg_wm_base);
    if (m_seat) wl_seat_destroy(m_seat);
    if (m_layer_shell) zwlr_layer_shell_v1_destroy(m_layer_shell);
    if (m_shm) wl_shm_destroy(m_shm);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry) wl_registry_destroy(m_registry);
    if (m_display) wl_display_disconnect(m_display);

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool AppEngine::init() {
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_wakeup_fd < 0) {
        std::cerr << "[miqutoolkit] Failed to create eventfd for AppEngine." << std::endl;
    }

    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        std::cerr << "[miqutoolkit] Failed to connect to Wayland display." << std::endl;
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        std::cerr << "[miqutoolkit] Failed to get Wayland registry." << std::endl;
        return false;
    }

    wl_registry_add_listener(m_registry, &s_registry_listener, this);
    wl_display_roundtrip(m_display);

    if (!m_compositor || !m_shm || (!m_layer_shell && !m_xdg_wm_base)) {
        std::cerr << "[miqutoolkit] Missing required Wayland globals (compositor, shm, and layer_shell or xdg_wm_base)." << std::endl;
        return false;
    }

    setup_config_watcher();

    return true;
}

struct zwlr_foreign_toplevel_manager_v1* AppEngine::get_foreign_toplevel_manager() {
    if (!m_foreign_toplevel_manager && m_foreign_toplevel_manager_name > 0 && m_registry) {
        m_foreign_toplevel_manager = static_cast<struct zwlr_foreign_toplevel_manager_v1*>(
            wl_registry_bind(m_registry, m_foreign_toplevel_manager_name, &zwlr_foreign_toplevel_manager_v1_interface, std::min(m_foreign_toplevel_manager_version, 3u)));
        WindowManager::get()->init_protocol(m_foreign_toplevel_manager);
        wl_display_roundtrip(m_display);
    }
    return m_foreign_toplevel_manager;
}

struct ext_workspace_manager_v1* AppEngine::get_workspace_manager_protocol() {
    if (!m_ext_workspace_manager && m_ext_workspace_manager_name > 0 && m_registry) {
        m_ext_workspace_manager = static_cast<struct ext_workspace_manager_v1*>(
            wl_registry_bind(m_registry, m_ext_workspace_manager_name, &ext_workspace_manager_v1_interface, 1));
        WorkspaceManager::get()->init_protocol(m_ext_workspace_manager);
        wl_display_roundtrip(m_display);
    }
    return m_ext_workspace_manager;
}

void AppEngine::registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<AppEngine*>(data);

    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        self->m_compositor = static_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm = static_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
        self->m_xdg_wm_base = static_cast<struct xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(self->m_xdg_wm_base, &s_wm_base_listener, self);
    } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        self->m_layer_shell = static_cast<struct zwlr_layer_shell_v1*>(
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        self->m_foreign_toplevel_manager_name = name;
        self->m_foreign_toplevel_manager_version = version;
    } else if (std::strcmp(interface, ext_workspace_manager_v1_interface.name) == 0) {
        self->m_ext_workspace_manager_name = name;
        self->m_ext_workspace_manager_version = version;
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        self->m_seat = static_cast<struct wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
        wl_seat_add_listener(self->m_seat, &s_seat_listener, self);
    } else if (std::strcmp(interface, ext_idle_notifier_v1_interface.name) == 0) {
        self->m_idle_notifier = static_cast<struct ext_idle_notifier_v1*>(
            wl_registry_bind(registry, name, &ext_idle_notifier_v1_interface, 1));
    } else if (std::strcmp(interface, ext_session_lock_manager_v1_interface.name) == 0) {
        self->m_session_lock_manager = static_cast<struct ext_session_lock_manager_v1*>(
            wl_registry_bind(registry, name, &ext_session_lock_manager_v1_interface, 1));
    } else if (std::strcmp(interface, wl_output_interface.name) == 0) {
        OutputManager::get()->handle_global(registry, name, interface, version);
    }
}

void AppEngine::registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
    OutputManager::get()->handle_global_remove(name);
}

void AppEngine::register_window(std::shared_ptr<Window> win) {
    m_windows.push_back(win);
    if (win && m_seat_capabilities != 0) {
        win->update_seat_capabilities(m_seat_capabilities);
    }
}

void AppEngine::unregister_window(std::shared_ptr<Window> win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it != m_windows.end()) {
        m_windows.erase(it);
    }
    if (m_quit_on_last_window && m_windows.empty()) {
        quit(0);
    }
}

void AppEngine::post(std::function<void()> task) {
    if (!task) return;
    {
        std::lock_guard<std::mutex> lock(m_tasks_mutex);
        m_posted_tasks.push_back(std::move(task));
    }
    if (m_wakeup_fd >= 0) {
        uint64_t val = 1;
        ssize_t s = write(m_wakeup_fd, &val, sizeof(val));
        (void)s;
    }
}

void AppEngine::request_redraw_all() {
    for (auto& win : m_windows) {
        if (win) {
            win->schedule_redraw();
        }
    }
}

int AppEngine::enter_loop() {
    m_running = true;
    int display_fd = wl_display_get_fd(m_display);

    while (m_running && (!m_quit_on_last_window || !m_windows.empty())) {
        while (wl_display_prepare_read(m_display) != 0) {
            wl_display_dispatch_pending(m_display);
        }
        wl_display_flush(m_display);

        struct pollfd pfd[3];
        pfd[0].fd = display_fd;
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;

        pfd[1].fd = m_wakeup_fd;
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;

        pfd[2].fd = m_inotify_fd;
        pfd[2].events = POLLIN;
        pfd[2].revents = 0;

        int nfds = 1;
        if (m_wakeup_fd >= 0) nfds = 2;
        if (m_inotify_fd >= 0) nfds = 3;

        int ret = poll(pfd, nfds, -1);

        if (ret < 0) {
            if (errno == EINTR) {
                wl_display_cancel_read(m_display);
                continue;
            }
            wl_display_cancel_read(m_display);
            break;
        }

        if (pfd[0].revents & POLLIN) {
            if (wl_display_read_events(m_display) < 0) {
                break;
            }
        } else {
            wl_display_cancel_read(m_display);
        }

        if (wl_display_dispatch_pending(m_display) < 0) {
            break;
        }

        if (m_wakeup_fd >= 0 && (pfd[1].revents & POLLIN)) {
            uint64_t val = 0;
            ssize_t s = read(m_wakeup_fd, &val, sizeof(val));
            (void)s;

            std::vector<std::function<void()>> tasks_to_run;
            {
                std::lock_guard<std::mutex> lock(m_tasks_mutex);
                tasks_to_run.swap(m_posted_tasks);
            }
            for (auto& t : tasks_to_run) {
                if (t) t();
            }
        }

        if (m_inotify_fd >= 0 && (pfd[2].revents & POLLIN)) {
            handle_inotify_events();
        }
    }
    return m_exit_code;
}

bool AppEngine::lock_session(std::function<void(bool success)> on_locked) {
    if (!m_session_lock_manager) {
        std::cerr << "[miqutoolkit] Compositor does not support ext-session-lock-v1" << std::endl;
        return false;
    }
    if (m_session_lock) {
        if (on_locked) on_locked(true);
        return true;
    }

    m_on_locked_cb = std::move(on_locked);
    m_session_lock = ext_session_lock_manager_v1_lock(m_session_lock_manager);
    if (!m_session_lock) {
        std::cerr << "[miqutoolkit] Failed to create ext_session_lock_v1" << std::endl;
        return false;
    }

    ext_session_lock_v1_add_listener(m_session_lock, &s_session_lock_listener, this);
    wl_display_flush(m_display);
    return true;
}

void AppEngine::unlock_session() {
    if (m_session_lock) {
        ext_session_lock_v1_unlock_and_destroy(m_session_lock);
        m_session_lock = nullptr;
        if (m_display) {
            wl_display_roundtrip(m_display);
        }
    }
}

void AppEngine::quit(int exit_code) {
    m_exit_code = exit_code;
    m_running = false;
    if (m_wakeup_fd >= 0) {
        uint64_t val = 1;
        ssize_t s = write(m_wakeup_fd, &val, sizeof(val));
        (void)s;
    }
}

void AppEngine::setup_config_watcher() {
    if (m_inotify_fd < 0) {
        m_inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (m_inotify_fd < 0) {
            std::cerr << "[miqutoolkit] Failed to create inotify instance for config watcher: " << strerror(errno) << std::endl;
            return;
        }
    }

    // Clean up existing watches
    for (const auto& [wd, fname] : m_inotify_watches) {
        if (wd >= 0) {
            inotify_rm_watch(m_inotify_fd, wd);
        }
    }
    m_inotify_watches.clear();

    namespace fs = std::filesystem;
    auto config = Config::get();
    std::vector<std::string> files_to_watch = config->get_loaded_files();

    std::string user_cfg_dir = FsUtils::get_user_config_dir("miqutoolkit");
    if (!user_cfg_dir.empty()) {
        std::string def_file = user_cfg_dir + "/miqutoolkit.conf";
        if (std::find(files_to_watch.begin(), files_to_watch.end(), def_file) == files_to_watch.end()) {
            files_to_watch.push_back(def_file);
        }
    }

    std::map<std::string, std::vector<std::string>> dir_to_files;
    for (const auto& fpath : files_to_watch) {
        fs::path p(fpath);
        fs::path pdir = p.parent_path();
        std::string fname = p.filename().string();
        if (fs::exists(pdir)) {
            dir_to_files[pdir.string()].push_back(fname);
            // Also watch grandparent directory for symlink swaps (e.g. current -> scheme)
            if (pdir.has_parent_path() && fs::exists(pdir.parent_path())) {
                dir_to_files[pdir.parent_path().string()].push_back(pdir.filename().string());
            }
        } else if (pdir.has_parent_path() && fs::exists(pdir.parent_path())) {
            // If parent doesn't exist yet, watch grandparent for parent directory creation
            dir_to_files[pdir.parent_path().string()].push_back(pdir.filename().string());
        }
    }

    for (const auto& [dir_path, fnames] : dir_to_files) {
        int wd = inotify_add_watch(m_inotify_fd, dir_path.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
        if (wd >= 0) {
            for (const auto& fn : fnames) {
                m_inotify_watches.emplace_back(wd, fn);
            }
        }
    }
}

void AppEngine::handle_inotify_events() {
    alignas(struct inotify_event) char buffer[4096];
    bool should_reload = false;

    while (true) {
        ssize_t len = read(m_inotify_fd, buffer, sizeof(buffer));
        if (len <= 0) break;

        for (char* ptr = buffer; ptr < buffer + len; ) {
            auto* event = reinterpret_cast<const struct inotify_event*>(ptr);
            if (event->len > 0) {
                std::string ev_name(event->name);
                for (const auto& [wd, fname] : m_inotify_watches) {
                    if (wd == event->wd && fname == ev_name) {
                        should_reload = true;
                        break;
                    }
                }
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    if (should_reload) {
        Config::get()->init_toolkit_defaults();
        setup_config_watcher();

        for (auto& win : m_windows) {
            if (win) {
                win->refresh_theme();
            }
        }

        for (auto& listener : m_theme_change_listeners) {
            if (listener) listener();
        }
    }
}

} // namespace miqu
