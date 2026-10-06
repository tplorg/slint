#pragma once

#include <slint.h>

struct Settings;

/// Owns the main window's size, position and maximized state: restores them
/// from the `Settings` on start-up and keeps them persisted as they change.
class WindowState
{
public:
    WindowState(slint::Window &window, Settings &settings);

    /// Applies the stored geometry. Without a stored size the window keeps the
    /// preferred size written in the .slint file.
    void restore() const;

    /// Captures the current geometry into the settings and saves them.
    void save();

    /// Polls the geometry on a timer and saves as soon as it changes. Waiting
    /// for the window to be closed is not enough: a process that gets killed
    /// instead (closing the console of a console-subsystem executable, Ctrl+C,
    /// a crash) would forget everything.
    void watch();

private:
    /// Copies the current geometry into the settings and reports whether it
    /// differs from what the previous capture left there.
    bool capture_changed();

    slint::Window &window_;
    Settings &settings_;
    slint::Timer timer_;
};
