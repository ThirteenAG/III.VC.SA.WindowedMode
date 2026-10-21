#pragma once
#include <asi_settings/asi_settings.hpp>

// The plugin's settings: described in settings/settings.json (embedded as a resource), stored in the .cfg next to the
// plugin. Read the values through Int and Bool; change them with Set, so that the .cfg and editors follow.
inline asi_settings::Plugin& Settings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}
