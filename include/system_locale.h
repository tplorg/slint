#pragma once

#include <string>

/// Returns the language the operating system is configured for, in the same
/// locale notation as the `translations/<locale>/LC_MESSAGES` directories, for
/// example "en_US" or "zh_CN".
///
/// Returns an empty string when the platform does not report a usable
/// language, in which case the application keeps its default language.
std::string system_locale();
