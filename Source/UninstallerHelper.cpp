#include "UninstallerHelper.h"

#if JUCE_MAC
#import <Foundation/Foundation.h>
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
bool UninstallerHelper::executeMacOSUninstall()
{
    juce::File userHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    // 1. Delete user-level files directly without requiring admin rights
    userHome.getChildFile("Library/Audio/Plug-Ins/VST3/RecRoll.vst3").deleteRecursively();
    userHome.getChildFile("Library/Audio/Plug-Ins/CLAP/RecRoll.clap").deleteRecursively();
    userHome.getChildFile("Library/Audio/Plug-Ins/Components/RecRoll.component").deleteRecursively();
    userHome.getChildFile("Library/Application Support/RecRoll").deleteRecursively();
    userHome.getChildFile("Library/Preferences/com.recrollaudio.recroll.plist").deleteFile();
    userHome.getChildFile("Library/Saved Application State/com.recrollaudio.recroll.savedState").deleteRecursively();

    // 2. Write standalone script to /tmp to avoid shell quotation issues inside AppleScript
    juce::File scriptFile("/tmp/recroll_uninstall.sh");
    juce::String scriptContent =
        "#!/bin/bash\n"
        "rm -rf '/Applications/RecRoll.app' 2>/dev/null || true\n"
        "rm -rf '/Library/Audio/Plug-Ins/VST3/RecRoll.vst3' 2>/dev/null || true\n"
        "rm -rf '/Library/Audio/Plug-Ins/CLAP/RecRoll.clap' 2>/dev/null || true\n"
        "rm -rf '/Library/Audio/Plug-Ins/Components/RecRoll.component' 2>/dev/null || true\n"
        "rm -rf '/Library/Application Support/RecRoll' 2>/dev/null || true\n"
        "rm -rf /Library/LaunchAgents/com.recrollaudio.* 2>/dev/null || true\n"
        "rm -rf /Library/LaunchDaemons/com.recrollaudio.* 2>/dev/null || true\n"
        "rm -rf /var/db/receipts/com.recrollaudio.* 2>/dev/null || true\n"
        "pkgutil --forget com.recrollaudio.recroll 2>/dev/null || true\n"
        "pkgutil --forget com.recrollaudio.recroll.vst3 2>/dev/null || true\n"
        "pkgutil --forget com.recrollaudio.recroll.clap 2>/dev/null || true\n"
        "pkgutil --forget com.recrollaudio.recroll.au 2>/dev/null || true\n"
        "pkgutil --forget com.recrollaudio.recroll.app 2>/dev/null || true\n"
        "killall -9 AudioComponentRegistrar 2>/dev/null || true\n"
        "rm -f /tmp/recroll_uninstall.sh 2>/dev/null || true\n"
        "exit 0\n";

    scriptFile.replaceWithText(scriptContent);
    scriptFile.setExecutePermission(true);

    juce::String appleScriptSource =
        "do shell script \"bash /tmp/recroll_uninstall.sh\" with administrator privileges";

    bool success = false;

    @autoreleasepool
    {
        NSString* src = [NSString stringWithUTF8String:appleScriptSource.toRawUTF8()];
        NSAppleScript* script = [[NSAppleScript alloc] initWithSource:src];
        NSDictionary* errorInfo = nil;
        NSAppleEventDescriptor* desc = [script executeAndReturnError:&errorInfo];

        if (errorInfo != nil)
        {
            NSNumber* errNum = [errorInfo objectForKey:NSAppleScriptErrorNumber];
            if (errNum != nil && [errNum intValue] == -128)
            {
                scriptFile.deleteFile();
                return false;
            }
        }

        if (desc != nil || errorInfo == nil)
        {
            success = true;
        }
    }

    if (!success)
    {
        juce::ChildProcess process;
        juce::StringArray args;
        args.add("/usr/bin/osascript");
        args.add("-e");
        args.add(appleScriptSource);

        if (process.start(args))
        {
            process.waitForProcessToFinish(60000);
            success = (process.getExitCode() == 0);
        }
    }

    scriptFile.deleteFile();
    return success;
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
