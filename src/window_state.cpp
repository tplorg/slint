#include "window_state.h"

#include "settings.h"

#include <chrono>
#include <cstdint>

namespace {

constexpr auto kPollInterval = std::chrono::milliseconds(500);

} // namespace

WindowState::WindowState(slint::Window &window, Settings &settings)
    : window_(window), settings_(settings)
{
}

void WindowState::restore() const
{
    if (settings_.window_width > 0 && settings_.window_height > 0) {
        window_.set_size(slint::PhysicalSize(
                { std::uint32_t(settings_.window_width), std::uint32_t(settings_.window_height) }));
        // Wayland and some tiling window managers place windows themselves and
        // ignore this.
        window_.set_position(slint::PhysicalPosition({ settings_.window_x, settings_.window_y }));
    }
    if (settings_.window_maximized) {
        window_.set_maximized(true);
    }
}

void WindowState::save()
{
    capture_changed();
    settings_.save();
}

void WindowState::watch()
{
    timer_.start(slint::TimerMode::Repeated, kPollInterval, [&] {
        if (capture_changed()) {
            settings_.save();
        }
    });
}

bool WindowState::capture_changed()
{
    const int x = settings_.window_x;
    const int y = settings_.window_y;
    const int width = settings_.window_width;
    const int height = settings_.window_height;
    const bool maximized = settings_.window_maximized;

    // A maximized window keeps the size and position stored earlier, so that
    // un-maximizing after a restart still gives the user the window they had
    // before.
    settings_.window_maximized = window_.is_maximized();
    if (!settings_.window_maximized) {
        const slint::PhysicalSize size = window_.size();
        const slint::PhysicalPosition position = window_.position();
        settings_.window_width = int(size.width);
        settings_.window_height = int(size.height);
        settings_.window_x = position.x;
        settings_.window_y = position.y;
    }

    return settings_.window_x != x || settings_.window_y != y
            || settings_.window_width != width || settings_.window_height != height
            || settings_.window_maximized != maximized;
}
