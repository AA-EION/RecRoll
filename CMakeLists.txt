cmake_minimum_required(VERSION 3.22)

project(RecRoll VERSION 1.0.0 LANGUAGES C CXX)

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

juce_add_plugin(RecRoll
    COMPANY_NAME "RecRollAudio"
    BUNDLE_ID "com.recrollaudio.recroll"
    PLUGIN_MANUFACTURER_CODE Rcll
    PLUGIN_CODE Rcrb
    FORMATS ${RECROLL_FORMATS}
    PRODUCT_NAME "RecRoll"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Configure CLAP format target
if (TARGET clap-juce-extensions)
    clap_juce_extensions_plugin(
        TARGET RecRoll
        CLAP_ID "com.recrollaudio.recroll"
        CLAP_FEATURES "audio-effect" "utility" "sampler"
    )
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
)

target_compile_definitions(RecRoll PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
)

target_link_libraries(RecRoll PRIVATE
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
