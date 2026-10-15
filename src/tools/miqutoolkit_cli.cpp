#include <miqutoolkit/core/config.hpp>
#include <iostream>
#include <string>

static void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Management and diagnostics utility for miqutoolkit.\n\n"
              << "Options:\n"
              << "  -h, --help            Show this help message and exit\n"
              << "  -v, --version         Show miqutoolkit version\n"
              << "  --init-config         Initialize ~/.config/miqutoolkit/miqutoolkit.conf from template\n"
              << "  --print-config        Display currently resolved global metrics and loaded files\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        print_usage(argv[0]);
        return 0;
    } else if (arg == "-v" || arg == "--version") {
        std::cout << "miqutoolkit 1.0.0\n";
        return 0;
    } else if (arg == "--init-config") {
        std::string res = miqu::Config::init_user_config("miqutoolkit", "miqutoolkit.conf");
        if (!res.empty()) {
            std::cout << "[miqutoolkit-cli] Configuration initialized at: " << res << "\n";
            return 0;
        } else {
            std::cout << "[miqutoolkit-cli] Configuration already exists or could not be created.\n";
            return 1;
        }
    } else if (arg == "--print-config") {
        auto cfg = miqu::Config::get();
        std::cout << "--- miqutoolkit Metrics ---\n"
                  << "Font Family:   " << cfg->metrics.font_family << "\n"
                  << "Font Size:     " << cfg->metrics.font_size << "pt\n"
                  << "Corner Radius: " << cfg->metrics.corner_radius << "px\n"
                  << "Border Width:  " << cfg->metrics.border_width << "px\n"
                  << "Elevation:     " << cfg->metrics.card_elevation << "\n"
                  << "Icon Theme:    " << cfg->metrics.icon_theme << "\n\n"
                  << "--- miqutoolkit Loaded Configuration Files ---\n";
        for (const auto& file : cfg->get_loaded_files()) {
            std::cout << " - " << file << "\n";
        }
        return 0;
    } else {
        std::cerr << "Unknown option: " << arg << "\n";
        print_usage(argv[0]);
        return 1;
    }
}
