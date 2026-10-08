#pragma once

#include <wayland-client.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace miqu {

class OutputManager;

struct OutputInfo {
    uint32_t id = 0;
    std::string name;
    std::string make;
    std::string model;
    std::string description;
    int x = 0;
    int y = 0;
    int physical_width = 0;
    int physical_height = 0;
    int width = 0;
    int height = 0;
    int refresh_rate = 0;
    int scale = 1;
    int transform = 0;
    struct wl_output* wl_output = nullptr;
};

class OutputManager {
public:
    static OutputManager* get();

    std::vector<OutputInfo> get_outputs() const;
    const OutputInfo* find_output_by_name(const std::string& name) const;
    const OutputInfo* find_output_by_wl_output(struct wl_output* out) const;

    void on_outputs_changed(std::function<void()> callback);
    void clear();

    void handle_global(struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    void handle_global_remove(uint32_t name);

private:
    OutputManager() = default;
    ~OutputManager();

    void notify_changed();

    struct InternalOutput {
        OutputInfo info;
        bool pending_done = false;
    };

    std::vector<std::shared_ptr<InternalOutput>> m_outputs;
    std::vector<std::function<void()>> m_listeners;

    static const struct wl_output_listener s_output_listener;
};

} // namespace miqu
