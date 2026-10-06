cmake_minimum_required(VERSION 3.21)
project(my_application LANGUAGES CXX)

# Platform-native look. Slint compiles its standard widgets in exactly one
# style, chosen here at build time: Fluent on Windows, Cupertino on macOS and
# iOS, Material on Android, and on Linux the Qt style when a Qt installation
# is found (Slint's own fallback is Fluent). The widget colors and the
# light/dark mode then follow the operating system. Override with
# -DSLINT_STYLE=<style> to pin one style on every platform.
# A pre-built Slint package only guarantees the style it was built with; if
# one of these styles fails to link against an installed package, drop the
# package and let the FetchContent fallback below build Slint from source.
if (NOT SLINT_STYLE)
    if (ANDROID)
        set(_native_style "material")
    elseif (WIN32)
        set(_native_style "fluent")
    elseif (APPLE)
        set(_native_style "cupertino")
    else ()
        set(_native_style "fluent")
        if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            # Slint only looks for Qt when its Qt feature is enabled, so do
            # the same lookup here first: this keeps the default Fluent when
            # no Qt is installed instead of failing the configure step.
            find_package(Qt6 6.2 QUIET COMPONENTS Core Widgets)
            if (NOT TARGET Qt::qmake)
                find_package(Qt5 5.15 QUIET COMPONENTS Core Widgets)
            endif ()
            if (TARGET Qt::qmake)
                set(_native_style "qt")
                set(SLINT_FEATURE_BACKEND_QT ON CACHE BOOL "Use Qt for widgets where available" FORCE)
            endif ()
        endif ()
    endif ()
    set(SLINT_STYLE "${_native_style}" CACHE STRING "The Slint widget style" FORCE)
endif ()

find_package(Slint QUIET)
if (NOT Slint_FOUND)
    message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
    include(FetchContent)
    FetchContent_Declare(
        Slint
        GIT_REPOSITORY https://github.com/slint-ui/slint.git
        # `release/1` will auto-upgrade to the latest Slint >= 1.0.0 and < 2.0.0
        # `release/1.0` will auto-upgrade to the latest Slint >= 1.0.0 and < 1.1.0
        GIT_TAG release/1
        SOURCE_SUBDIR api/cpp
    )
    FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

set(MY_APPLICATION_SOURCES
    src/main.cpp
    src/settings.cpp
    src/system_locale.cpp
    src/window_state.cpp
)

if(ANDROID)
    add_library(my_application SHARED ${MY_APPLICATION_SOURCES})
else()
    add_executable(my_application ${MY_APPLICATION_SOURCES})
endif()
target_include_directories(my_application PRIVATE include)
target_link_libraries(my_application PRIVATE Slint::Slint)

if(APPLE)
    # src/system_locale.cpp reads the user's language with CFLocale.
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    target_link_libraries(my_application PRIVATE ${COREFOUNDATION_FRAMEWORK})
endif()

if(WIN32 AND NOT ANDROID)
    # The icon that Explorer and the task switcher show for the executable file,
    # next to the window icon in ui/app-window.slint.
    enable_language(RC)
    target_sources(my_application PRIVATE assets/windows/app.rc)
endif()

slint_target_sources(my_application ui/app-window.slint)

# The translations in translations/<locale>/LC_MESSAGES/<domain>.po are compiled
# into the binary, so that no .mo files or gettext runtime are needed. They are
# selected at run time with slint::select_bundled_translation(), see src/main.cpp.
set_target_properties(my_application PROPERTIES
    SLINT_TRANSLATION_DOMAIN app
    SLINT_BUNDLE_TRANSLATIONS "${CMAKE_CURRENT_SOURCE_DIR}/translations"
)

# On Windows, copy the Slint DLL next to the application binary so that it's found.
if (WIN32)
    add_custom_command(TARGET my_application POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:my_application> $<TARGET_FILE_DIR:my_application> COMMAND_EXPAND_LISTS)
endif()
