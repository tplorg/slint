# Contributing

Thanks for your interest in improving this template. It demonstrates how to build
a multi-platform C++ application with [Slint](https://slint.dev) for the UI and
CMake for the build system, and contributions of any size are welcome.

## Ways to contribute

- Report a bug or request a feature through a GitHub issue.
- Improve the C++ code in `src/` and `include/`, or the `.slint` files in `ui/`.
- Add or fix a translation in `translations/`.
- Improve this document or the `README.md`.

For anything larger than a small fix, please open an issue first so we can agree
on the approach before you invest time in a pull request.

## Prerequisites

- [CMake](https://cmake.org/download/) 3.21 or newer.
- A C++ 20 compiler (for example MSVC 2022, GCC 10, or Clang 10).
- [Rust](https://www.rust-lang.org/learn/get-started), only when Slint is built
  from source. `CMakeLists.txt` downloads and compiles Slint through
  `FetchContent` when it cannot find an installed package.

On Linux or Windows on x86-64 you can instead install a pre-built Slint package
and skip the Rust toolchain. See
<https://slint.dev/docs/cpp/cmake.html#install-binary-packages>.

## Building and running

The repository ships CMake presets for the common configurations:

```sh
# Debug
cmake --preset debug
cmake --build --preset debug

# Release
cmake --preset release
cmake --build --preset release
```

Without presets:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The resulting binary is named `my_application`. On multi-config generators
(Visual Studio, Xcode) it lives in a per-configuration subdirectory such as
`build/Release/`.

When you use Visual Studio Code, open the repository as a folder, accept the
recommended extensions from `.vscode/extensions.json`, and build or debug with
the CMake Tools integration and the `Debug` launch configuration.

### Android

The `android/` directory contains a Gradle wrapper around the same CMake
project. After installing the Android SDK, NDK, CMake, and the Rust Android
targets described in `README.md`:

```sh
cd android
./gradlew installDebug
```

## Project layout

```
CMakeLists.txt              build definition and installed/bundled resources
src/                        C++ sources (main, settings, system locale, window state)
include/                    public C++ headers
ui/                         Slint markup
  app-window.slint          application shell, built on the std-widgets TabWidget
  globals/                  app-wide globals: the preferences C++ drives
  components/               reusable UI components
  pages/                    one file per tab
translations/               gettext catalogs, one per locale
assets/                     images and other runtime resources
android/                    Gradle wrapper for the Android build
.github/workflows/ci.yml    continuous integration
```

## Coding guidelines

Follow the style already used in the repository; it is the one the existing
files were written in.

### C++

- Target C++ 20.
- Use 4 spaces for indentation, never tabs.
- Use `#pragma once` in headers.
- Naming: types in `PascalCase`, free functions in `snake_case`, constants in
  `kCamelCase`, variables and members in `snake_case`.
- Document public declarations and non-obvious logic with `///` comments.
- Keep platform-specific code inside the existing `#if defined(...)` blocks.
- Prefer the standard library; avoid adding a new dependency for something that
  can be done with what is already there.

### Slint

- Use 4 spaces for indentation; file names are `kebab-case`.
- Component and global names are `PascalCase`; properties and callbacks are
  `kebab-case`.
- Every user-visible string goes through `@tr("...")` so it can be translated.
- The look of the application is the platform's: prefer `std-widgets.slint`
  components and `Palette` brushes over hand-drawn controls and fixed colors,
  so the widget style chosen in `CMakeLists.txt` (Fluent, Cupertino, Material
  or Qt) stays in charge.
- Keep the C++/Slint contract in sync. For example, the order of `kThemeOptions`
  in `src/main.cpp` must match the theme `ComboBox` model in
  `ui/pages/settings.slint`, and the language codes in
  `ui/globals/app-settings.slint` must match the translation directories.

## Translations

Translations are compiled into the binary at build time, so no `.mo` files or
gettext runtime are needed at run time. To add a language:

1. Mark new user-visible strings in the `.slint` files with `@tr("...")`.
2. Create `translations/<locale>/LC_MESSAGES/app.po`, using an existing catalog
   as a template. The directory name is the locale (for example `de` or
   `pt_BR`).
3. Register the display name and the locale code in `language-names` and
   `language-codes` in `ui/globals/app-settings.slint`.

Every entry needs a `msgctxt`: Slint uses the enclosing component or global name
as the default context, and a missing context leaves the string untranslated.
The translation domain (`app`) and the locale directories are configured through
the `SLINT_TRANSLATION_DOMAIN` and `SLINT_BUNDLE_TRANSLATIONS` target properties
in `CMakeLists.txt`.

## Tests

There is currently no automated test suite; CI only configures and builds the
project in Release mode on Linux, macOS, and Windows.

Because of that, please verify your change manually before opening a pull
request:

- The project configures and builds for your platform.
- The application starts and the pages load.
- Switching the theme and the language takes effect and survives a restart,
  including the window size, position, and maximized state.
- With "Run in background" enabled, the tray icon is there while the application
  runs, closing the window hides it to that icon, and the icon shows the window
  again or quits; the selection survives a restart. With it disabled there is no
  icon and closing the window quits.
- Any `.slint` change renders as intended at both sides of the 760px navigation
  breakpoint.

## Submitting a pull request

1. Fork the repository and create a branch from `main`.
2. Keep the change focused on one topic, and match the surrounding style.
3. Write a description that explains what changed, why, and how it was tested.
4. Make sure the CI workflow passes.
5. Update `README.md` or this document when your change affects how the project
   is used or built.

Use a short, imperative subject line for commits (for example
"Persist the window geometry on close").

`build/` and user-specific CMake presets are ignored; don't commit generated
artifacts.

## License

This project is released under the MIT License, see `LICENSE`. By submitting a
contribution you agree that it is licensed under the same terms.