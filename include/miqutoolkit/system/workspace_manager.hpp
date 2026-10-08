#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

struct ext_workspace_handle_v1;
struct ext_workspace_manager_v1;
struct ext_workspace_group_handle_v1;
struct ext_workspace_handle_v1_listener;
struct ext_workspace_manager_v1_listener;

namespace miqu {

class WorkspaceManager;

struct WorkspaceInfo {
    size_t id = 1;
    std::string name;
    bool is_active = false;
    bool is_empty = true;
    bool is_urgent = false;

    void activate();

    struct ext_workspace_handle_v1* get_handle() const { return handle; }

private:
    friend class WorkspaceManager;
    struct ext_workspace_handle_v1* handle = nullptr;
    WorkspaceManager* manager = nullptr;
};

class WorkspaceManager {
public:
    static WorkspaceManager* get();

    std::vector<WorkspaceInfo> get_workspaces() const;
    const WorkspaceInfo* get_active_workspace() const;

    void activate_workspace(size_t id);
    void on_workspaces_changed(std::function<void()> callback);

    bool is_supported() const { return m_manager != nullptr; }

    void init_protocol(struct ext_workspace_manager_v1* mgr);
    void clear();

private:
    friend struct WorkspaceInfo;
    WorkspaceManager() = default;
    ~WorkspaceManager();

    void notify_changed();

    struct ext_workspace_manager_v1* m_manager = nullptr;
    std::vector<std::shared_ptr<WorkspaceInfo>> m_workspaces;
    std::vector<std::function<void()>> m_listeners;

    static const struct ::ext_workspace_manager_v1_listener s_manager_listener;
    static const struct ::ext_workspace_handle_v1_listener s_handle_listener;
};

} // namespace miqu
