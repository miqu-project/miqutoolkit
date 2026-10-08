#include "miqutoolkit/core/shm_pool.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

namespace miqu {

const struct wl_buffer_listener ShmPool::s_buffer_listener = {
    .release = buffer_release,
};

void ShmPool::buffer_release(void* data, struct wl_buffer* wl_buffer) {
    auto* buf = static_cast<Buffer*>(data);
    if (buf) {
        buf->busy = false;
    }
}

static int create_anonymous_file(off_t size) {
    int fd = memfd_create("miqutoolkit-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd >= 0) {
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK);
    } else {
        char template_name[] = "/tmp/miqutoolkit-shm-XXXXXX";
        fd = mkstemp(template_name);
        if (fd >= 0) {
            unlink(template_name);
            long flags = fcntl(fd, F_GETFD);
            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        }
    }

    if (fd < 0) return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

ShmPool::ShmPool(struct wl_shm* shm, int width, int height)
    : m_shm(shm), m_width(width), m_height(height) {
    create_buffer(m_buffers[0]);
    create_buffer(m_buffers[1]);
}

ShmPool::~ShmPool() {
    destroy_buffer(m_buffers[0]);
    destroy_buffer(m_buffers[1]);
}

void ShmPool::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    if (width <= 0 || height <= 0) return;

    destroy_buffer(m_buffers[0]);
    destroy_buffer(m_buffers[1]);

    m_width = width;
    m_height = height;

    create_buffer(m_buffers[0]);
    create_buffer(m_buffers[1]);
    m_current_buffer = 0;
}

bool ShmPool::create_buffer(Buffer& buf) {
    if (m_width <= 0 || m_height <= 0 || !m_shm) return false;

    int stride = m_width * 4;
    int size = stride * m_height;

    int fd = create_anonymous_file(size);
    if (fd < 0) return false;

    buf.data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf.data == MAP_FAILED) {
        close(fd);
        buf.data = nullptr;
        return false;
    }
    buf.size = size;

    struct wl_shm_pool* pool = wl_shm_create_pool(m_shm, fd, size);
    if (!pool) {
        munmap(buf.data, size);
        buf.data = nullptr;
        buf.size = 0;
        close(fd);
        return false;
    }

    buf.wl_buf = wl_shm_pool_create_buffer(pool, 0, m_width, m_height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    if (!buf.wl_buf) {
        munmap(buf.data, size);
        buf.data = nullptr;
        buf.size = 0;
        return false;
    }

    buf.cairo_surf = cairo_image_surface_create_for_data(
        static_cast<unsigned char*>(buf.data),
        CAIRO_FORMAT_ARGB32,
        m_width,
        m_height,
        stride
    );

    if (!buf.cairo_surf || cairo_surface_status(buf.cairo_surf) != CAIRO_STATUS_SUCCESS) {
        destroy_buffer(buf);
        return false;
    }

    buf.cr = cairo_create(buf.cairo_surf);
    if (!buf.cr || cairo_status(buf.cr) != CAIRO_STATUS_SUCCESS) {
        destroy_buffer(buf);
        return false;
    }
    buf.busy = false;

    wl_buffer_add_listener(buf.wl_buf, &s_buffer_listener, &buf);
    return true;
}

void ShmPool::destroy_buffer(Buffer& buf) {
    if (buf.cr) {
        cairo_destroy(buf.cr);
        buf.cr = nullptr;
    }
    if (buf.cairo_surf) {
        cairo_surface_destroy(buf.cairo_surf);
        buf.cairo_surf = nullptr;
    }
    if (buf.wl_buf) {
        wl_buffer_destroy(buf.wl_buf);
        buf.wl_buf = nullptr;
    }
    if (buf.data && buf.data != MAP_FAILED) {
        if (buf.size > 0) {
            munmap(buf.data, buf.size);
        }
        buf.data = nullptr;
        buf.size = 0;
    }
    buf.busy = false;
}

ShmPool::Buffer* ShmPool::get_next_buffer() {
    Buffer* buf = &m_buffers[m_current_buffer];
    if (buf->busy) {
        m_current_buffer = (m_current_buffer + 1) % 2;
        buf = &m_buffers[m_current_buffer];
    }
    m_current_buffer = (m_current_buffer + 1) % 2;
    return buf;
}

} // namespace miqu
