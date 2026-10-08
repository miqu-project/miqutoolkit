#include "miqutoolkit/system/workspace_manager.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "ext-workspace-v1-client-protocol.h"
#include <algorithm>
#include <iostream>

namespace miqu {

WorkspaceManager* WorkspaceManager::get() {
    static WorkspaceManager s_instance;
    auto* engine = AppEngine::instance();
    if (engine && !s_instance.m_manager) {
        engine->get_workspace_manager_protocol();
    }
    return &s_instance;
}

void WorkspaceManager::clear() {
    for (auto& ws : m_workspaces) {
        if (ws && ws->handle) {
            ext_workspace_handle_v1_destroy(ws->handle);
            ws->handle = nullptr;
        }
    }
    m_workspaces.clear();
    m_listeners.clear();
    m_manager = nullptr;
}

WorkspaceManager::~WorkspaceManager() {
    clear();
}

void WorkspaceInfo::activate() {
    if (!handle || !manager || !manager->m_manager) return;
    ext_workspace_handle_v1_activate(handle);
    ext_workspace_manager_v1_commit(manager->m_manager);
    auto* engine = AppEngine::instance();
    if (engine) {
        wl_display_flush(engine->get_display());
    }
}

const struct ::ext_workspace_handle_v1_listener WorkspaceManager::s_handle_listener = {
    .id = [](void*, struct ext_workspace_handle_v1*, const char*) {},
    .name = [](void* data, struct ext_workspace_handle_v1*, const char* name) {
        auto* ws = static_cast<WorkspaceInfo*>(data);
        ws->name = name ? name : "";
        try {
            if (!ws->name.empty()) {
                ws->id = std::stoull(ws->name);
            }
        } catch (...) {}
    },
    .coordinates = [](void*, struct ext_workspace_handle_v1*, struct wl_array*) {},
    .state = [](void* data, struct ext_workspace_handle_v1*, uint32_t state) {
        auto* ws = static_cast<WorkspaceInfo*>(data);
        ws->is_active = (state & EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE) != 0;
        ws->is_urgent = (state & EXT_WORKSPACE_HANDLE_V1_STATE_URGENT) != 0;
        ws->is_empty  = (state & EXT_WORKSPACE_HANDLE_V1_STATE_HIDDEN) != 0;
    },
    .capabilities = [](void*, struct ext_workspace_handle_v1*, uint32_t) {},
    .removed = [](void* data, struct ext_workspace_handle_v1* handle) {
        auto* ws = static_cast<WorkspaceInfo*>(data);
        WorkspaceManager* mgr = ws ? ws->manager : nullptr;
        ext_workspace_handle_v1_destroy(handle);
        if (mgr) {
            auto& list = mgr->m_workspaces;
            list.erase(std::remove_if(list.begin(), list.end(),
                [ws](const std::shared_ptr<WorkspaceInfo>& w) { return w.get() == ws; }),
                list.end());
            mgr->notify_changed();
        }
    }
};

const struct ::ext_workspace_manager_v1_listener WorkspaceManager::s_manager_listener = {
    .workspace_group = [](void*, struct ext_workspace_manager_v1*, struct ext_workspace_group_handle_v1*) {},
    .workspace = [](void* data, struct ext_workspace_manager_v1*, struct ext_workspace_handle_v1* handle) {
        auto* self = static_cast<WorkspaceManager*>(data);
        auto ws = std::make_shared<WorkspaceInfo>();
        ws->handle = handle;
        ws->manager = self;
        self->m_workspaces.push_back(ws);
        ext_workspace_handle_v1_add_listener(handle, &s_handle_listener, ws.get());
    },
    .done = [](void* data, struct ext_workspace_manager_v1*) {
        auto* self = static_cast<WorkspaceManager*>(data);
        std::sort(self->m_workspaces.begin(), self->m_workspaces.end(),
            [](const auto& a, const auto& b) { return a->id < b->id; });
        self->notify_changed();
    },
    .finished = [](void* data, struct ext_workspace_manager_v1*) {
        auto* self = static_cast<WorkspaceManager*>(data);
        self->m_manager = nullptr;
    }
};

void WorkspaceManager::init_protocol(struct ext_workspace_manager_v1* mgr) {
    if (m_manager == mgr) return;
    m_manager = mgr;
    if (m_manager) {
        ext_workspace_manager_v1_add_listener(m_manager, &s_manager_listener, this);
    }
}

std::vector<WorkspaceInfo> WorkspaceManager::get_workspaces() const {
    std::vector<WorkspaceInfo> result;
    result.reserve(m_workspaces.size());
    for (const auto& ws : m_workspaces) {
        if (ws) {
            result.push_back(*ws);
        }
    }
    return result;
}

const WorkspaceInfo* WorkspaceManager::get_active_workspace() const {
    for (const auto& ws : m_workspaces) {
        if (ws && ws->is_active) {
            return ws.get();
        }
    }
    return nullptr;
}

void WorkspaceManager::activate_workspace(size_t id) {
    for (auto& ws : m_workspaces) {
        if (ws && ws->id == id) {
            ws->activate();
            break;
        }
    }
}

void WorkspaceManager::on_workspaces_changed(std::function<void()> callback) {
    if (callback) {
        m_listeners.push_back(std::move(callback));
    }
}

void WorkspaceManager::notify_changed() {
    for (const auto& cb : m_listeners) {
        if (cb) cb();
    }
}

} // namespace miqu
