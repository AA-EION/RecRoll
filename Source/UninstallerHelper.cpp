#include "UninstallerHelper.h"

#if JUCE_MAC
#include <cstdlib>
#elif JUCE_WINDOWS
#include <windows.h>
#endif

namespace RecRoll
{

bool UninstallerHelper::isUninstallerAvailable()
{
#if JUCE_MAC || JUCE_WINDOWS
    return true;
#else
    return false;
#endif
}

void UninstallerHelper::promptAndExecuteUninstall(juce::Component* parentComponent)
{
    juce::String title = "Uninstall RecRoll Completely";
    juce::String message =
        "Are you sure you want to completely uninstall RecRoll from this system?\n\n"
        "This will remove:\n"
        "- The RecRoll Standalone Application\n"
        "- RecRoll VST3, CLAP, and AU plugins\n"
        "- Application Support, cache, and preference files\n"
        "- System package installer receipts\n\n"
        "This operation cannot be undone.";

    auto options = juce::MessageBoxOptions::makeOptionsYesNo(title, message)
        .withButtonLabels("Uninstall", "Cancel")
        .withParentComponent(parentComponent);

    juce::AlertWindow::showAsync(options, [](int result)
    {
        if (result != 1) // 1 = Yes / Uninstall
            return;

#if JUCE_MAC
        bool success = executeMacOSUninstall();
        if (success)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Uninstallation Complete",
                "RecRoll and all associated plugins and files have been removed.\nThe app will now exit.",
                "OK",
                nullptr,
                juce::ModalCallbackFunction::create([](int)
                {
                    juce::JUCEApplicationBase::quit();
                }));
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Uninstallation Cancelled or Failed",
                "Uninstallation was not completed. If administrator privileges were denied, please try again.");
        }
#elif JUCE_WINDOWS
        bool success = executeWindowsUninstall();
        if (!success)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Uninstaller Not Found",
                "Could not automatically launch the Windows uninstaller.\nYou can uninstall RecRoll from Windows Settings -> Installed Apps, or run installer/windows/uninstall.ps1.");
        }
#endif
    });
}

#if JUCE_MAC
bool UninstallerHelper::executeMacOSUninstall()
{
    // Build multi-line bash cleanup script executed via osascript with administrator privileges
    juce::String script =
        "do shell script \""
        "rm -rf '/Applications/RecRoll.app' ; "
        "rm -rf '/Library/Audio/Plug-Ins/VST3/RecRoll.vst3' ; "
        "rm -rf ~/Library/Audio/Plug-Ins/VST3/RecRoll.vst3 ; "
        "rm -rf '/Library/Audio/Plug-Ins/CLAP/RecRoll.clap' ; "
        "rm -rf ~/Library/Audio/Plug-Ins/CLAP/RecRoll.clap ; "
        "rm -rf '/Library/Audio/Plug-Ins/Components/RecRoll.component' ; "
        "rm -rf ~/Library/Audio/Plug-Ins/Components/RecRoll.component ; "
        "rm -rf '/Library/Application Support/RecRoll' ; "
        "rm -rf ~/Library/Application\\ Support/RecRoll ; "
        "rm -rf ~/Library/Preferences/com.recrollaudio.* ; "
        "rm -rf ~/Library/Caches/com.recrollaudio.* ; "
        "rm -rf ~/Library/Saved\\ Application\\ State/com.recrollaudio.* ; "
        "rm -rf /Library/LaunchAgents/com.recrollaudio.* ; "
        "rm -rf ~/Library/LaunchAgents/com.recrollaudio.* ; "
        "rm -rf /Library/LaunchDaemons/com.recrollaudio.* ; "
        "pkgutil --forget com.recrollaudio.recroll 2>/dev/null || true ; "
        "rm -f /var/db/receipts/com.recrollaudio.* 2>/dev/null || true ; "
        "killall -9 AudioComponentRegistrar 2>/dev/null || true ; "
        "echo done"
        "\" with administrator privileges";

    // Escape quotes for command line invocation
    juce::ChildProcess process;
    juce::StringArray args;
    args.add("osascript");
    args.add("-e");
    args.add(script);

    if (process.start(args))
    {
        process.waitForProcessToFinish(60000);
        return process.getExitCode() == 0;
    }

    return false;
}
#endif

#if JUCE_WINDOWS
bool UninstallerHelper::executeWindowsUninstall()
{
    // Try to locate Inno Setup uninstaller in standard installation directory
    juce::File progFiles = juce::File::getSpecialLocation(juce::File::commonProgramFilesDirectory)
                               .getParentDirectory()
                               .getChildFile("RecRoll");

    auto uninsExe = progFiles.getChildFile("unins000.exe");
    if (!uninsExe.existsAsFile())
    {
        // Try 64-bit Program Files
        progFiles = juce::File("C:\\Program Files\\RecRoll");
        uninsExe = progFiles.getChildFile("unins000.exe");
    }

    if (uninsExe.existsAsFile())
    {
        uninsExe.startAsProcess();
        juce::JUCEApplicationBase::quit();
        return true;
    }

    return false;
}
#endif

} // namespace RecRoll
