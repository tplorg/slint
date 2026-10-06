#pragma once

#include <string>

/// The user's preferences, kept across restarts in a small text file inside the
/// platform's configuration directory, or next to the executable when that
/// directory cannot be written.
///
/// An empty `theme` means "follow the system", otherwise it is one of "light"
/// or "dark". An empty `language` means "follow the system", otherwise it is a
/// locale such as "zh_CN".
struct Settings
{
    std::string theme;
    std::string language;

    // Closing the window hides it to the system tray instead of quitting.
    bool background_mode = true;

    // The window geometry, in physical pixels. A width or height of 0 means that
    // nothing has been stored yet, and the window starts at the preferred size
    // written in the .slint file.
    int window_x = 0;
    int window_y = 0;
    int window_width = 0;
    int window_height = 0;
    bool window_maximized = false;

    /// Reads the settings file. A missing or unreadable file yields defaults.
    static Settings load();

    /// Writes the settings file. Does nothing when no location can be written -
    /// see src/settings.cpp for where the file lives.
    void save() const;
};
