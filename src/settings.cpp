#include "settings.h"

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace {

/// Change this to your application name: it names the configuration directory.
constexpr const char *kApplicationName = "slint-template";

/// The file inside the platform's configuration directory.
std::filesystem::path configured_file()
{
#if defined(_WIN32)
    if (const char *appdata = std::getenv("APPDATA"); appdata && *appdata) {
        return std::filesystem::path(appdata) / kApplicationName / "settings.ini";
    }
#elif defined(__APPLE__)
    if (const char *home = std::getenv("HOME"); home && *home) {
        return std::filesystem::path(home) / "Library" / "Application Support" / kApplicationName
                / "settings.ini";
    }
#elif defined(__ANDROID__)
    // Android's app-private directory is only known to the Java side, so it
    // needs JNI to be reached from C++. Settings are per-session until then;
    // see the README.
    return {};
#else
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return std::filesystem::path(xdg) / kApplicationName / "settings.ini";
    }
    if (const char *home = std::getenv("HOME"); home && *home) {
        return std::filesystem::path(home) / ".config" / kApplicationName / "settings.ini";
    }
#endif
    return {};
}

/// The directory of the running executable, empty when it cannot be found.
std::filesystem::path executable_dir()
{
#if defined(_WIN32)
    wchar_t buffer[4096];
    const unsigned long length = ::GetModuleFileNameW(nullptr, buffer, 4096);
    if (length == 0 || length >= 4096) {
        return {};
    }
    return std::filesystem::path(buffer).parent_path();
#elif defined(__APPLE__)
    char buffer[4096];
    unsigned long length = sizeof(buffer);
    if (::_NSGetExecutablePath(buffer, &length) != 0) {
        return {};
    }
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    char buffer[4096];
    const long length = ::readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (length <= 0 || length >= long(sizeof(buffer))) {
        return {};
    }
    return std::filesystem::path(std::string_view(buffer, size_t(length))).parent_path();
#else
    return {};
#endif
}

/// Whether \a path can be written, creating it and its directories.
bool writable(std::filesystem::path path)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    return bool(std::ofstream(path, std::ios::app));
}

/// Where the preferences are kept, decided once:
///
///   * `SLINT_TEMPLATE_SETTINGS` names a file to use, for a portable install or
///     to keep the preferences inside a sandbox;
///   * otherwise the platform's configuration directory, when it can be written;
///   * otherwise next to the executable, for the installs - restricted by a
///     sandbox or a read-only home - that cannot write there.
const std::filesystem::path &settings_file()
{
    static const std::filesystem::path path = [] {
        if (const char *override_path = std::getenv("SLINT_TEMPLATE_SETTINGS");
                override_path && *override_path) {
            return std::filesystem::path(override_path);
        }
        if (const auto configured = configured_file(); writable(configured)) {
            return configured;
        }
        const auto dir = executable_dir();
        return dir.empty() ? std::filesystem::path {} : dir / "settings.ini";
    }();
    return path;
}

std::string_view trim(std::string_view value)
{
    constexpr std::string_view whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}

/// Parses a whole number, falling back to \a fallback for anything else.
int to_int(std::string_view value, int fallback)
{
    int result = 0;
    const char *const first = value.data();
    const char *const last = first + value.size();
    const auto [end, error] = std::from_chars(first, last, result);
    return error == std::errc {} && end == last ? result : fallback;
}

} // namespace

Settings Settings::load()
{
    Settings settings;

    std::ifstream file(settings_file());
    for (std::string line; std::getline(file, line);) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        // Substring views stay valid as long as they point into `line` itself.
        const std::string_view line_view = line;
        const std::string_view key = trim(line_view.substr(0, separator));
        const std::string_view value = trim(line_view.substr(separator + 1));
        if (key == "theme") {
            settings.theme = value;
        } else if (key == "language") {
            settings.language = value;
        } else if (key == "background-mode") {
            settings.background_mode = value == "1";
        } else if (key == "window-x") {
            settings.window_x = to_int(value, 0);
        } else if (key == "window-y") {
            settings.window_y = to_int(value, 0);
        } else if (key == "window-width") {
            settings.window_width = to_int(value, 0);
        } else if (key == "window-height") {
            settings.window_height = to_int(value, 0);
        } else if (key == "window-maximized") {
            settings.window_maximized = value == "1";
        }
    }
    return settings;
}

void Settings::save() const
{
    const auto &path = settings_file();
    if (path.empty()) {
        return;
    }

    // Opened in the usual way: `settings_file()` already picked a location that
    // can be written, and a file that goes missing in between - a removed
    // directory, an unmounted drive - is not worth reporting on every resize.
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return;
    }
    file << "theme=" << theme << "\n";
    file << "language=" << language << "\n";
    file << "background-mode=" << (background_mode ? 1 : 0) << "\n";
    file << "window-x=" << window_x << "\n";
    file << "window-y=" << window_y << "\n";
    file << "window-width=" << window_width << "\n";
    file << "window-height=" << window_height << "\n";
    file << "window-maximized=" << (window_maximized ? 1 : 0) << "\n";
}
