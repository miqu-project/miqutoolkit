# miqutoolkit

<p align="center">
  <b>Modern C++20 UI Framework & Application Development Toolkit for Wayland</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B20-blue?style=flat-square" alt="C++20" />
  <img src="https://img.shields.io/badge/Renderer-Cairo%20%2F%20Pango-orange?style=flat-square" alt="Cairo/Pango" />
  <img src="https://img.shields.io/badge/Protocol-Wayland%20Layer--Shell-purple?style=flat-square" alt="Layer-Shell" />
  <img src="https://img.shields.io/badge/License-Apache--2.0-green?style=flat-square" alt="License" />
</p>

---

`miqutoolkit` is the foundational UI framework powering all desktop applications in the **Miqu Desktop Ecosystem** (`miqubar`, `miqudesk`, `miqumusic`, `miqulauncher`, `miqunotify`, `miqutheme`, `miqusecure`, etc.). It abstracts Cairo vector drawing, Pango typography, and Wayland surface protocols into a clean, declarative C++ widget system.

---

## ✨ Features & Architecture

* **Declarative Layout System**: Intuitive Android-inspired view hierarchy (`LinearLayout`, `GridLayout`, `RelativeLayout`, `CardView`, `TextView`, `ImageView`, `Button`, `ScrollView`).
* **Dual Surface Modes**:
  * **Application Windows (`AppWindow`)**: Standard floating or tiled Wayland client surfaces.
  * **Layer Shell Surfaces (`LayerSurface`)**: Desktop panels, status bars, notification drawers, lockscreens, and background canvases using the `wlr-layer-shell` protocol.
* **Uniform Two-Tier Configuration**:
  * **System Baseline**: Runs 100% out of the box from `/usr/share/miqutoolkit/miqutoolkit.conf`.
  * **User Customization**: Generated on demand via `init_user_config()` with safe timestamped archiving (`.bak.<YYYYMMDD-HHMMSS>`).
  * **Decoupled Dynamic Theming**: Transparently sources active wallpaper/palettes via `source = ~/.config/miqutheme/current/miqutoolkit.conf`.
* **Atomic Staging Live Reload**: Inotify watcher parses changes into an isolated staging buffer before committing, guaranteeing zero transient UI flicker on theme updates.

---

## 🛠️ Quick C++ Example

Creating a native Wayland layer-shell status pill in ~30 lines:

```cpp
#include <miqutoolkit/miqutoolkit.hpp>

int main(int argc, char** argv) {
    auto engine = miqu::AppEngine::create();

    auto window = engine->create_layer_surface("my-panel", miqu::Layer::Top);
    window->set_anchor(miqu::Anchor::Top | miqu::Anchor::Left | miqu::Anchor::Right);
    window->set_exclusive_zone(36);

    auto root = std::make_shared<miqu::LinearLayout>(miqu::Orientation::Horizontal);
    root->set_padding(8, 16);
    root->set_background_color(miqu::Config::get()->colors.surface);

    auto title = std::make_shared<miqu::TextView>("Miqu Desktop");
    title->set_font_size(miqu::Config::get()->metrics.h2_size);
    title->set_text_color(miqu::Config::get()->colors.on_surface);

    root->add_view(title);
    window->set_root_view(root);

    return engine->run();
}
```

---

## 🔨 Building & Installation

Every repository in the Miqu ecosystem follows the uniform `make.sh` build pattern:

### 1. Build Locally (Unprivileged)
```bash
./make.sh
```
Compiles `libmiqutoolkit.so` into the local `build/` directory for syntax verification and testing.

### 2. System-Wide Installation
```bash
sudo ./make.sh
```
Configures, compiles, and installs headers to `/usr/include`, shared library to `/usr/lib`, and baseline configuration to `/usr/share/miqutoolkit/miqutoolkit.conf`.

---

## 📄 License
Licensed under the Apache License, Version 2.0.
