// Entry point: applies the stored preferences to the UI and keeps them in sync
// with the user's choices.
//
// Three application-level preferences are demonstrated:
//   * the theme - follow the system, or force light/dark - applied to the
//     std-widgets `Palette` global;
//   * the interface language - follow the system, or an explicit one - applied
//     through the gettext translations bundled into the binary at build time;
//   * background mode - whether closing the window hides the application to
//     the system tray icon defined in ui/app-window.slint (AppTray), which
//     shows the window again or quits, instead of quitting directly.
// All three are persisted with `Settings` (see src/settings.cpp), and the
// window geometry is kept in the same file by `WindowState` (see
// src/window_state.cpp).

#include "app-window.h"
#include "settings.h"
#include "system_locale.h"
#include "window_state.h"

#include <slint.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

namespace {

/// The C++ representation of the `[string]` properties in `AppSettings`.
using StringList = std::shared_ptr<slint::Model<slint::SharedString>>;

/// Mirrors the theme ComboBox model in ui/pages/settings.slint, in the same order.
struct ThemeOption
{
    const char *name;
    slint::language::ColorScheme scheme;
};

constexpr ThemeOption kThemeOptions[] = {
    // ColorScheme::Unknown hands the choice back to the operating system.
    { "auto", slint::language::ColorScheme::Unknown },
    { "light", slint::language::ColorScheme::Light },
    { "dark", slint::language::ColorScheme::Dark },
};

/// Index of the "follow the system" entry in the preference ComboBoxes.
constexpr int kAutoIndex = 0;

std::string to_string(const slint::SharedString &value)
{
    return std::string(std::string_view(value));
}

/// The language part of a locale: "zh_CN" -> "zh".
std::string_view language_of(std::string_view locale)
{
    return locale.substr(0, locale.find('_'));
}

/// Index of a stored theme name, falling back to "follow the system".
int theme_index(std::string_view name)
{
    for (int i = 0; i < int(std::size(kThemeOptions)); ++i) {
        if (name == kThemeOptions[i].name) {
            return i;
        }
    }
    return kAutoIndex;
}

/// Index of \a code in the language ComboBox model: an exact locale match
/// ("zh_CN") first, then a match on the language alone ("en_US" -> "en"), and
/// kAutoIndex ("follow the system") when this build has no translation for it.
int language_index(const StringList &codes, std::string_view code)
{
    if (code.empty()) {
        return kAutoIndex;
    }
    // Index 0 is "follow the system" and never a translation.
    int match = kAutoIndex;
    for (std::size_t i = 1; i < codes->row_count(); ++i) {
        const auto row = codes->row_data(i);
        if (!row) {
            continue;
        }
        const std::string_view candidate = *row;
        if (candidate == code) {
            return int(i);
        }
        if (match == kAutoIndex && language_of(candidate) == language_of(code)) {
            match = int(i);
        }
    }
    return match;
}

/// Resolves a language preference to the translation to load: an explicit
/// choice wins, an empty one follows the system.
int resolve_language(const StringList &codes, std::string_view preference)
{
    return language_index(codes, preference.empty() ? system_locale() : preference);
}

/// Loads the bundled translation at \a index. kAutoIndex loads the language the
/// strings in the .slint files are written in.
void select_language(const StringList &codes, int index)
{
    if (std::size_t(index) < codes->row_count()) {
        if (const auto code = codes->row_data(std::size_t(index))) {
            slint::select_bundled_translation(std::string_view(*code));
        }
    }
}

} // namespace

#ifdef __ANDROID__
extern "C" void slint_main()
#else
int main(int argc, char **argv)
#endif
{
    auto ui = AppWindow::create();
    auto &app_settings = ui->global<AppSettings>();
    const auto language_codes = app_settings.get_language_codes();

    ui->on_request_increase_value([&] { ui->set_counter(ui->get_counter() + 1); });

    Settings settings = Settings::load();

    // Apply the stored preferences before the window is shown, so that it never
    // appears in the wrong theme or language.
    const int theme = theme_index(settings.theme);
    app_settings.set_theme_mode_index(theme);
    ui->global<Palette>().set_color_scheme(kThemeOptions[theme].scheme);

    app_settings.set_language_index(language_index(language_codes, settings.language));
    select_language(language_codes, resolve_language(language_codes, settings.language));

    app_settings.set_background_mode(settings.background_mode);

    // Same for the window: the size and the position from the last run.
    WindowState window_state { ui->window(), settings };
    window_state.restore();

    // Writes the preferences, including the current window geometry.
    const auto save_settings = [&] { window_state.save(); };

    // The tray icon that keeps the application running in the background. It is
    // created and shown before the event loop starts, because the native icon is
    // registered as the component is created and only an icon that is visible
    // then keeps the loop alive once the window is hidden: shown later, the
    // application quits with the window instead of going to the tray.
    auto tray = AppTray::create();
    tray->on_show_window([&] { ui->window().show(); });
    tray->on_quit([&] {
        save_settings();
        slint::quit_event_loop();
    });
    // The icon is the way back from the background, so it goes with the mode.
    if (!app_settings.get_background_mode()) {
        tray->hide();
    }

    // Then react to what the user picks in the UI.
    app_settings.on_theme_mode_changed([&] {
        const ThemeOption &option = kThemeOptions[app_settings.get_theme_mode_index()];
        ui->global<Palette>().set_color_scheme(option.scheme);
        settings.theme = option.name;
        save_settings();
    });

    app_settings.on_language_changed([&](const slint::SharedString &code) {
        settings.language = to_string(code);
        select_language(language_codes, resolve_language(language_codes, settings.language));
        save_settings();
    });

    app_settings.on_background_mode_changed([&](bool enabled) {
        settings.background_mode = enabled;
        if (enabled) {
            tray->show();
        } else {
            tray->hide();
        }
        save_settings();
    });

    // And remember the geometry when the user closes the window. In background
    // mode the window only hides and the tray icon takes over, keeping the
    // application running until the window is shown again or the user quits;
    // otherwise the application quits with it.
    ui->window().on_close_requested([&] {
        save_settings();
        if (!app_settings.get_background_mode()) {
            // The loop quits with the last window only when no tray icon is left
            // visible, so ask for it directly rather than relying on that.
            slint::quit_event_loop();
            return slint::CloseRequestResponse::HideWindow;
        }
        // A visible tray icon keeps the event loop alive on its own, so hiding
        // the window hides the application. Where no tray is available - a
        // platform without one, or a session that refuses the icon - the loop
        // ends with the window, rather than leaving the application running with
        // no way back to it.
        ui->window().hide();
        return slint::CloseRequestResponse::KeepWindowShown;
    });

    // Also write it as soon as it changes, before a clean close.
    window_state.watch();

    ui->run();
}
