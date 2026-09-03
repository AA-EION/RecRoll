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

    auto options = juce::MessageBoxOptions()
        .withIconType(juce::MessageBoxIconType::QuestionIcon)
        .withTitle(title)
        .withMessage(message)
        .withButton("Uninstall")
        .withButton("Cancel")
        .withParentComponent(parentComponent);

    juce::AlertWindow::showAsync(options, [](int result)
    {
        if (result != 1) // 1 = Yes / Uninstall
            return;

#if JUCE_MAC
        bool success = executeMacOSUninstall();
        if (success)
        {
            auto okOptions = juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::InfoIcon)
                .withTitle("Uninstallation Complete")
                .withMessage("RecRoll and all associated plugins and files have been removed.\nThe app will now exit.")
                .withButton("OK");

            juce::AlertWindow::showAsync(okOptions, [](int)
            {
                juce::JUCEApplicationBase::quit();
            });
        }
        else
        {
            auto errOptions = juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Uninstallation Cancelled or Failed")
                .withMessage("Uninstallation was not completed. If administrator privileges were denied, please try again.")
                .withButton("OK");

            juce::AlertWindow::showAsync(errOptions, nullptr);
        }
#elif JUCE_WINDOWS
        bool success = executeWindowsUninstall();
        if (!success)
        {
            auto errOptions = juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Uninstaller Not Found")
                .withMessage("Could not automatically launch the Windows uninstaller.\nYou can uninstall RecRoll from Windows Settings -> Installed Apps, or run installer/windows/uninstall.ps1.")
                .withButton("OK");

            juce::AlertWindow::showAsync(errOptions, nullptr);
        }
#endif
    });
}

#if JUCE_MAC
#import <Foundation/Foundation.h>

bool UninstallerHelper::executeMacOSUninstall()
{
    juce::File tempScript = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("recroll_uninstall.sh");

    juce::String scriptContent =
        "#!/bin/bash\n"
        "rm -rf '/Applications/RecRoll.app'\n"
        "rm -rf '/Library/Audio/Plug-Ins/VST3/RecRoll.vst3'\n"
        "rm -rf ~/Library/Audio/Plug-Ins/VST3/RecRoll.vst3\n"
        "rm -rf '/Library/Audio/Plug-Ins/CLAP/RecRoll.clap'\n"
        "rm -rf ~/Library/Audio/Plug-Ins/CLAP/RecRoll.clap\n"
        "rm -rf '/Library/Audio/Plug-Ins/Components/RecRoll.component'\n"
        "rm -rf ~/Library/Audio/Plug-Ins/Components/RecRoll.component\n"
        "rm -rf '/Library/Application Support/RecRoll'\n"
        "rm -rf ~/Library/Application\\ Support/RecRoll\n"
        "rm -rf ~/Library/Preferences/com.recrollaudio.*\n"
        "rm -rf ~/Library/Caches/com.recrollaudio.*\n"
        "rm -rf ~/Library/Saved\\ Application\\ State/com.recrollaudio.*\n"
        "rm -rf /Library/LaunchAgents/com.recrollaudio.*\n"
        "rm -rf ~/Library/LaunchAgents/com.recrollaudio.*\n"
        "rm -rf /Library/LaunchDaemons/com.recrollaudio.*\n"
        "pkgutil --forget com.recrollaudio.recroll 2>/dev/null || true\n"
        "rm -f /var/db/receipts/com.recrollaudio.* 2>/dev/null || true\n"
        "killall -9 AudioComponentRegistrar 2>/dev/null || true\n"
        "exit 0\n";

    tempScript.replaceWithText(scriptContent);
    tempScript.setExecutePermission(true);

    juce::String appleScriptSource =
        "do shell script \"/bin/bash '" + tempScript.getFullPathName() + "'\" with administrator privileges";

    @autoreleasepool
    {
        NSString* src = [NSString stringWithUTF8String:appleScriptSource.toRawUTF8()];
        NSAppleScript* script = [[NSAppleScript alloc] initWithSource:src];
        NSDictionary* errorInfo = nil;
        NSAppleEventDescriptor* desc = [script executeAndReturnError:&errorInfo];

        tempScript.deleteFile();

        return (desc != nil && errorInfo == nil);
    }
}
#endif

#if JUCE_WINDOWS
bool UninstallerHelper::executeWindowsUninstall()
{
    // Try to locate Inno Setup uninstaller in standard installation directory
    juce::File progFiles = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                               .getChildFile("RecRoll");

    auto uninsExe = progFiles.getChildFile("unins000.exe");
    if (!uninsExe.existsAsFile())
    {
        // Fallback check
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
