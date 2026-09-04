cmake_minimum_required(VERSION 3.22)

project(RecRoll VERSION 1.1.0 LANGUAGES C CXX)

if (APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform targets
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    # Universal 2 Binary (Apple Silicon + Intel)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    endif()
elseif (WIN32)
    # Target Windows 10+
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

include(FetchContent)

# Fetch JUCE 8
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Fetch CLAP JUCE Extensions (with submodules for clap headers)
FetchContent_Declare(
    clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        main
    GIT_SUBMODULES_RECURSE ON
)
FetchContent_MakeAvailable(clap-juce-extensions)

# Define plugin formats
set(RECROLL_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND RECROLL_FORMATS AU)
endif()

# RecRoll is a product of EION STUDIOS, engineered by ISSEN Software Group.
# COMPANY_NAME is what a DAW prints in its plugin browser's Manufacturer column,
# so it carries the studio name verbatim; PLUGIN_MANUFACTURER_CODE is the
# four-character AU/VST identity that goes with it.
juce_add_plugin(RecRoll
    COMPANY_NAME "EION Studios"
    COMPANY_WEBSITE "https://eionstudios.com"
    COMPANY_EMAIL "contactus@eionstudios.com"
    BUNDLE_ID "com.eionstudios.recroll"
    PLUGIN_MANUFACTURER_CODE Eion
    PLUGIN_CODE Rcrb
    FORMATS ${RECROLL_FORMATS}
    PRODUCT_NAME "RecRoll"
    DESCRIPTION "Rolling sampler and retrospective recorder"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/RecRoll_Icon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/RecRoll_Icon_128.png"
    VST3_CATEGORIES "Fx" "Tools"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "RecRoll requires audio input access to capture and record rolling audio."
)

# Configure CLAP format target.
# FetchContent names this dependency "clap-juce-extensions", but the CMake target
# it defines is "clap_juce_extensions" with underscores. Testing the hyphenated
# name silently skipped this block, so no RecRoll_CLAP target was ever created
# and the CLAP artefact the installers ask for never got built.
if (TARGET clap_juce_extensions)
    clap_juce_extensions_plugin(
        TARGET RecRoll
        CLAP_ID "com.eionstudios.recroll"
        CLAP_MANUAL_URL "https://github.com/AA-EION/RecRoll"
        CLAP_SUPPORT_URL "https://github.com/AA-EION/RecRoll/issues"
        CLAP_FEATURES "audio-effect" "utility" "sampler"
    )
else()
    message(WARNING "clap_juce_extensions target not found - the CLAP format will not be built.")
endif()

target_sources(RecRoll PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/RollingBuffer.h
    Source/RollingBuffer.cpp
    Source/WaveformComponent.h
    Source/WaveformComponent.cpp
    Source/AudioExportHelper.h
    Source/AudioExportHelper.cpp
    Source/UninstallerHelper.h
    Source/UninstallerHelper.cpp
    Source/AboutComponent.h
    Source/AboutComponent.cpp
)

# Brand and application artwork compiled into every format, so the About panel
# renders identically in the standalone app and inside a host.
juce_add_binary_data(RecRollBrandAssets
    HEADER_NAME BrandAssets.h
    NAMESPACE BrandAssets
    SOURCES
        Resources/RecRoll_Icon.svg
        Resources/brand/EION_Simbolo_Blanco.svg
)

target_compile_definitions(RecRoll PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
)

target_link_libraries(RecRoll PRIVATE
    RecRollBrandAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Ad-hoc code signing and Objective-C++ configuration on macOS
if (APPLE)
    set_source_files_properties(Source/UninstallerHelper.cpp PROPERTIES LANGUAGE OBJCXX)
    add_custom_command(TARGET RecRoll_Standalone POST_BUILD
        COMMAND codesign --force --deep -s - "$<TARGET_FILE_DIR:RecRoll_Standalone>/../.."
        COMMENT "Ad-hoc code signing RecRoll Standalone app for Gatekeeper"
    )
    if (TARGET RecRoll_VST3)
        add_custom_command(TARGET RecRoll_VST3 POST_BUILD
            COMMAND codesign --force --deep -s - "$<TARGET_FILE_DIR:RecRoll_VST3>/../.."
            COMMENT "Ad-hoc code signing RecRoll VST3 bundle"
        )
    endif()
    if (TARGET RecRoll_AU)
        add_custom_command(TARGET RecRoll_AU POST_BUILD
            COMMAND codesign --force --deep -s - "$<TARGET_FILE_DIR:RecRoll_AU>/../.."
            COMMENT "Ad-hoc code signing RecRoll AU bundle"
        )
    endif()
    if (TARGET RecRoll_CLAP)
        add_custom_command(TARGET RecRoll_CLAP POST_BUILD
            COMMAND codesign --force --deep -s - "$<TARGET_FILE_DIR:RecRoll_CLAP>/../.."
            COMMENT "Ad-hoc code signing RecRoll CLAP bundle"
        )
    endif()
endif()
