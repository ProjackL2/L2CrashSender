cmake_minimum_required(VERSION 3.20)

project(L2CrashSender
    VERSION 1.0.0
    DESCRIPTION "Crash Report Sender"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Only for Windows
if(WIN32)
    # Create the executable
    add_executable(L2CrashSender
        "logger.cpp"
        "logger.h"
        "main.cpp"
        "main.h"
     )

    # Set target properties
    set_target_properties(L2CrashSender PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(L2CrashSender PRIVATE wininet)
    
    # MSVC specific settings
    if(MSVC)
        target_compile_options(L2CrashSender PRIVATE
            /W4          # Warning level 4
            /WX          # Warnings as errors
            /permissive- # Strict conformance mode
            /Zc:__cplusplus # Enable __cplusplus macro
            /utf-8       # Use UTF-8 for source and execution character sets
        )
        
        # Debug configuration
        target_compile_options(L2CrashSender PRIVATE
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        )
        
        # Release configuration
        target_compile_options(L2CrashSender PRIVATE
            $<$<CONFIG:Release>:/O2 /DNDEBUG>
        )
        
        # Set subsystem to console
        set_target_properties(L2CrashSender PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:CONSOLE"
        )
    endif()
    
    # Enable Unicode
    target_compile_definitions(L2CrashSender PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    # Include current directory for log.hpp
    target_include_directories(L2CrashSender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()


