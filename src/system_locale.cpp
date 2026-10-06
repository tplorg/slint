#include "system_locale.h"

#include <algorithm>

#if defined(_WIN32)
#    include <windows.h>
#elif defined(__APPLE__)
#    include <CoreFoundation/CoreFoundation.h>
#else
#    include <cstdlib>
#    if defined(__ANDROID__) && __has_include(<sys/system_properties.h>)
#        include <sys/system_properties.h>
#        define SLINT_TEMPLATE_HAVE_SYSTEM_PROPERTIES 1
#    endif
#endif

namespace {

/// Turns a platform locale into the "language_TERRITORY" spelling used by the
/// translation directories: "de-DE" and "de_DE.UTF-8" both become "de_DE".
std::string normalize(std::string locale)
{
    if (const auto encoding = locale.find_first_of(".@"); encoding != std::string::npos) {
        locale.erase(encoding);
    }
    std::replace(locale.begin(), locale.end(), '-', '_');
    return locale;
}

std::string platform_locale()
{
#if defined(_WIN32)
    // The user interface language, as opposed to the regional format.
    const LANGID language = GetUserDefaultUILanguage();
    wchar_t name[LOCALE_NAME_MAX_LENGTH] = {};
    if (LCIDToLocaleName(MAKELCID(language, SORT_DEFAULT), name, LOCALE_NAME_MAX_LENGTH, 0) == 0) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(size) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8.data(), size, nullptr, nullptr);
    return utf8;
#elif defined(__APPLE__)
    std::string result;
    if (CFLocaleRef locale = CFLocaleCopyCurrent()) {
        if (CFStringRef identifier = CFLocaleGetIdentifier(locale)) {
            char buffer[128] = {};
            if (CFStringGetCString(identifier, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                result = buffer;
            }
        }
        CFRelease(locale);
    }
    return result;
#else
#    ifdef SLINT_TEMPLATE_HAVE_SYSTEM_PROPERTIES
    // Android keeps the user's language in a system property. This is not part
    // of the stable NDK API: if your NDK no longer provides the header, read
    // `ALocaleManager` (<android/locale.h>) or `java.util.Locale` over JNI
    // instead, see the README.
    char property[PROP_VALUE_MAX] = {};
    if (__system_property_get("persist.sys.locale", property) > 0) {
        return property;
    }
#    endif
    // POSIX: the same environment variables gettext consults, most specific
    // first.
    for (const char *name : { "LC_ALL", "LC_MESSAGES", "LANG" }) {
        if (const char *value = std::getenv(name); value && *value) {
            return value;
        }
    }
    return {};
#endif
}

} // namespace

std::string system_locale()
{
    return normalize(platform_locale());
}
