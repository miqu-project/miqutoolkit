#pragma once

#include <wayland-client.h>
#include <cairo.h>
#include <cstdint>
#include <cstddef>

namespace miqu {

class ShmPool {
public:
    ShmPool(struct wl_shm* shm, int width, int height);
    ~ShmPool();

    void resize(int width, int height);

    struct Buffer {
        struct wl_buffer* wl_buf = nullptr;
        cairo_surface_t* cairo_surf = nullptr;
        cairo_t* cr = nullptr;
        void* data = nullptr;
        size_t size = 0;
        bool busy = false;
    };

    Buffer* get_next_buffer();
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

private:
    static void buffer_release(void* data, struct wl_buffer* wl_buffer);
    static const struct wl_buffer_listener s_buffer_listener;

    bool create_buffer(Buffer& buf);
    void destroy_buffer(Buffer& buf);

    struct wl_shm* m_shm = nullptr;
    int m_width = 0;
    int m_height = 0;

    Buffer m_buffers[2];
    int m_current_buffer = 0;
};

} // namespace miqu
