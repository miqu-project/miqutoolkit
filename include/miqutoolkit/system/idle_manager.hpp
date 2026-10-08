#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include <vector>

namespace miqu {

class AppEngine;

struct IdleListenerHandle {
    uint32_t id = 0;
    uint32_t timeout_ms = 0;
    std::function<void()> on_idle;
    std::function<void()> on_resume;
    void* internal_notification = nullptr;
};

class IdleManager {
public:
    static IdleManager* get();

    void init(AppEngine* engine);

    uint32_t add_listener(uint32_t timeout_sec, std::function<void()> on_idle, std::function<void()> on_resume);
    uint32_t add_listener_ms(uint32_t timeout_ms, std::function<void()> on_idle, std::function<void()> on_resume);
    void remove_listener(uint32_t id);
    void clear_listeners();

    bool is_supported() const;

private:
    IdleManager() = default;
    ~IdleManager();

    AppEngine* m_engine = nullptr;
    uint32_t m_next_id = 1;
    std::vector<std::shared_ptr<IdleListenerHandle>> m_listeners;
};

} // namespace miqu
