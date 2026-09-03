#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace RecRoll
{

class UninstallerHelper
{
public:
    /**
     * Checks whether uninstall feature is supported on the current platform/runtime.
     * Standalone builds on macOS and Windows support full GUI uninstallation.
     */
    static bool isUninstallerAvailable();

    /**
     * Shows a GUI confirmation modal and executes full system uninstallation.
     * On macOS: Prompts via native administrator dialog and removes:
     * - /Applications/RecRoll.app
     * - VST3, CLAP, and AU plugins (system & user Library)
     * - Application Support, Caches, Saved States, and Preferences
     * - LaunchAgents
     * - Package receipts (pkgutil --forget)
     * - Resets AudioComponentRegistrar cache
     *
     * On Windows: Launches Inno Setup uninstaller or cleans registry and program files.
     */
    static void promptAndExecuteUninstall(juce::Component* parentComponent);

private:
#if JUCE_MAC
    static bool executeMacOSUninstall();
#elif JUCE_WINDOWS
    static bool executeWindowsUninstall();
#endif
};

} // namespace RecRoll
