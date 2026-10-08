#include "miqutoolkit/system/idle_manager.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "ext-idle-notify-v1-client-protocol.h"
#include <algorithm>
#include <iostream>

namespace miqu {

static const struct ext_idle_notification_v1_listener s_idle_notification_listener = {
    .idled = [](void* data, struct ext_idle_notification_v1*) {
        auto* handle = static_cast<IdleListenerHandle*>(data);
        if (handle && handle->on_idle) {
            handle->on_idle();
        }
    },
    .resumed = [](void* data, struct ext_idle_notification_v1*) {
        auto* handle = static_cast<IdleListenerHandle*>(data);
        if (handle && handle->on_resume) {
            handle->on_resume();
        }
    }
};

IdleManager* IdleManager::get() {
    static IdleManager s_instance;
    return &s_instance;
}

void IdleManager::init(AppEngine* engine) {
    m_engine = engine;
}

bool IdleManager::is_supported() const {
    auto* eng = m_engine ? m_engine : AppEngine::instance();
    return eng && eng->get_idle_notifier() != nullptr;
}

uint32_t IdleManager::add_listener(uint32_t timeout_sec, std::function<void()> on_idle, std::function<void()> on_resume) {
    return add_listener_ms(timeout_sec * 1000, std::move(on_idle), std::move(on_resume));
}

uint32_t IdleManager::add_listener_ms(uint32_t timeout_ms, std::function<void()> on_idle, std::function<void()> on_resume) {
    auto* eng = m_engine ? m_engine : AppEngine::instance();
    if (!eng) {
        std::cerr << "[miqutoolkit::IdleManager] Error: AppEngine instance not found\n";
        return 0;
    }

    auto* notifier = eng->get_idle_notifier();
    auto* seat = eng->get_seat();
    if (!notifier || !seat) {
        std::cerr << "[miqutoolkit::IdleManager] Error: ext_idle_notifier_v1 or wl_seat not available from compositor\n";
        return 0;
    }

    auto handle = std::make_shared<IdleListenerHandle>();
    handle->id = m_next_id++;
    handle->timeout_ms = timeout_ms;
    handle->on_idle = std::move(on_idle);
    handle->on_resume = std::move(on_resume);

    struct ext_idle_notification_v1* notif = ext_idle_notifier_v1_get_idle_notification(notifier, timeout_ms, seat);
    if (!notif) {
        std::cerr << "[miqutoolkit::IdleManager] Error: Failed to create idle notification for timeout " << timeout_ms << "ms\n";
        return 0;
    }

    handle->internal_notification = notif;
    ext_idle_notification_v1_add_listener(notif, &s_idle_notification_listener, handle.get());

    m_listeners.push_back(handle);
    return handle->id;
}

void IdleManager::remove_listener(uint32_t id) {
    auto it = std::find_if(m_listeners.begin(), m_listeners.end(), [id](const std::shared_ptr<IdleListenerHandle>& h) {
        return h->id == id;
    });

    if (it != m_listeners.end()) {
        auto* notif = static_cast<struct ext_idle_notification_v1*>((*it)->internal_notification);
        if (notif) {
            ext_idle_notification_v1_destroy(notif);
        }
        m_listeners.erase(it);
    }
}

void IdleManager::clear_listeners() {
    for (auto& handle : m_listeners) {
        if (handle && handle->internal_notification) {
            auto* notif = static_cast<struct ext_idle_notification_v1*>(handle->internal_notification);
            ext_idle_notification_v1_destroy(notif);
            handle->internal_notification = nullptr;
        }
    }
    m_listeners.clear();
}

IdleManager::~IdleManager() {
    clear_listeners();
}

} // namespace miqu
