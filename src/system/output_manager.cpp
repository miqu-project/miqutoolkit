#include "miqutoolkit/system/output_manager.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace miqu {

const struct wl_output_listener OutputManager::s_output_listener = {
    .geometry = [](void* data, struct wl_output*, int32_t x, int32_t y,
                   int32_t physical_width, int32_t physical_height, int32_t,
                   const char* make, const char* model, int32_t transform) {
        auto* internal = static_cast<InternalOutput*>(data);
        internal->info.x = x;
        internal->info.y = y;
        internal->info.physical_width = physical_width;
        internal->info.physical_height = physical_height;
        if (make) internal->info.make = make;
        if (model) internal->info.model = model;
        internal->info.transform = transform;
    },
    .mode = [](void* data, struct wl_output*, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        auto* internal = static_cast<InternalOutput*>(data);
        if (flags & WL_OUTPUT_MODE_CURRENT) {
            internal->info.width = width;
            internal->info.height = height;
            internal->info.refresh_rate = refresh;
        }
    },
    .done = [](void* data, struct wl_output*) {
        auto* internal = static_cast<InternalOutput*>(data);
        internal->pending_done = true;
        OutputManager::get()->notify_changed();
    },
    .scale = [](void* data, struct wl_output*, int32_t factor) {
        auto* internal = static_cast<InternalOutput*>(data);
        internal->info.scale = factor > 0 ? factor : 1;
    },
    .name = [](void* data, struct wl_output*, const char* name) {
        auto* internal = static_cast<InternalOutput*>(data);
        if (name) internal->info.name = name;
    },
    .description = [](void* data, struct wl_output*, const char* description) {
        auto* internal = static_cast<InternalOutput*>(data);
        if (description) internal->info.description = description;
    }
};

OutputManager* OutputManager::get() {
    static OutputManager s_instance;
    return &s_instance;
}

OutputManager::~OutputManager() {
    clear();
}

void OutputManager::clear() {
    for (auto& item : m_outputs) {
        if (item && item->info.wl_output) {
            wl_output_destroy(item->info.wl_output);
            item->info.wl_output = nullptr;
        }
    }
    m_outputs.clear();
    m_listeners.clear();
}

std::vector<OutputInfo> OutputManager::get_outputs() const {
    std::vector<OutputInfo> result;
    for (const auto& item : m_outputs) {
        if (item) {
            result.push_back(item->info);
        }
    }
    return result;
}

const OutputInfo* OutputManager::find_output_by_name(const std::string& name) const {
    for (const auto& item : m_outputs) {
        if (item && item->info.name == name) {
            return &item->info;
        }
    }
    return nullptr;
}

const OutputInfo* OutputManager::find_output_by_wl_output(struct wl_output* out) const {
    for (const auto& item : m_outputs) {
        if (item && item->info.wl_output == out) {
            return &item->info;
        }
    }
    return nullptr;
}

void OutputManager::on_outputs_changed(std::function<void()> callback) {
    if (callback) {
        m_listeners.push_back(std::move(callback));
    }
}

void OutputManager::notify_changed() {
    for (auto& cb : m_listeners) {
        if (cb) cb();
    }
}

void OutputManager::handle_global(struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    if (std::strcmp(interface, wl_output_interface.name) != 0) return;

    auto internal = std::make_shared<InternalOutput>();
    internal->info.id = name;
    internal->info.wl_output = static_cast<struct wl_output*>(
        wl_registry_bind(registry, name, &wl_output_interface, std::min(version, 4u))
    );

    if (!internal->info.wl_output) return;

    wl_output_add_listener(internal->info.wl_output, &s_output_listener, internal.get());
    m_outputs.push_back(internal);
}

void OutputManager::handle_global_remove(uint32_t name) {
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if ((*it)->info.id == name) {
            if ((*it)->info.wl_output) {
                wl_output_destroy((*it)->info.wl_output);
            }
            m_outputs.erase(it);
            notify_changed();
            break;
        }
    }
}

} // namespace miqu
