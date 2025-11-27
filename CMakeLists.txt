cmake_minimum_required(VERSION 3.16)

project(CYThread
    VERSION 1.0.0
    LANGUAGES CXX
)

# ---- Options -----------------------------------------------------------------

option(CYTHREAD_BUILD_SHARED "Build CYThread as a shared library" ON)
option(CYTHREAD_BUILD_TESTS  "Build CYThread tests" OFF)

# ---- Global settings ---------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Multi-config generators (MSVC / Xcode) already define Debug/Release etc.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ---- Platform detection macros (mirror existing headers) ---------------------

if(WIN32)
    add_compile_definitions(
        CYTHREAD_WIN_OS
        _WINDOWS
        _USRDLL
    )
elseif(APPLE)
    # Distinguish macOS / iOS based on CMake SDK when possible
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_OSX_SYSROOT MATCHES "iphoneos" OR CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        add_compile_definitions(CYTHREAD_IOS_OS)
    else()
        add_compile_definitions(CYTHREAD_MAC_OS)
    endif()
elseif(UNIX)
    add_compile_definitions(CYTHREAD_UNIX_OS)
endif()

# ---- Sources & headers -------------------------------------------------------

set(CYTHREAD_PUBLIC_HEADERS
    Inc/CYThread/CYThreadDefine.hpp
    Inc/CYThread/CYThreadFactory.hpp
    Inc/CYThread/ICYThread.hpp
)

set(CYTHREAD_PRIVATE_HEADERS
    Src/CYPlatformSpecifier.hpp
    Src/CYSystemDesc.hpp
    Src/CYThread.hpp
    Src/CYThreadFoundation.hpp
    Src/CYThreadPCH.hpp
    Src/CYThreadPool.hpp
    Src/CYThreadPoolProperties.hpp
    Src/CYThreadProperties.hpp
    Src/CYThreadWindows.hpp
)

set(CYTHREAD_SOURCES
    Src/CYSystemDesc.cpp
    Src/CYThread.cpp
    Src/CYThreadFactory.cpp
    Src/CYThreadFoundation.cpp
    Src/CYThreadPCH.cpp
    Src/CYThreadPool.cpp
    Src/CYThreadPoolProperties.cpp
    Src/CYThreadProperties.cpp
    Src/CYThreadWindows.cpp
)

# ---- Library target ----------------------------------------------------------

if(CYTHREAD_BUILD_SHARED)
    add_library(CYThread SHARED
        ${CYTHREAD_SOURCES}
        ${CYTHREAD_PUBLIC_HEADERS}
        ${CYTHREAD_PRIVATE_HEADERS}
    )
    target_compile_definitions(CYThread PRIVATE CYTHREAD_EXPORTS)
else()
    add_library(CYThread STATIC
        ${CYTHREAD_SOURCES}
        ${CYTHREAD_PUBLIC_HEADERS}
        ${CYTHREAD_PRIVATE_HEADERS}
    )
endif()

add_library(CYThread::CYThread ALIAS CYThread)

target_include_directories(CYThread
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
        ${CMAKE_CURRENT_SOURCE_DIR}/Build
)

# Windows-specific settings (mirror vcxproj)
if(WIN32)
    target_compile_definitions(CYThread
        PRIVATE
            WIN32
    )

    # Optional: match /W3 and SDL, equivalent flags
    if(MSVC)
        target_compile_options(CYThread PRIVATE /W3 /sdl)
    endif()
endif()

# Threads & platform libs ------------------------------------------------------

find_package(Threads REQUIRED)
target_link_libraries(CYThread
    PUBLIC
        Threads::Threads
)

if(WIN32)
    target_link_libraries(CYThread
        PRIVATE
            kernel32
            user32
            advapi32
    )
elseif(APPLE)
    string(TOLOWER "${CMAKE_OSX_SYSROOT}" CYTHREAD_OSX_SYSROOT_LOWER)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CYTHREAD_OSX_SYSROOT_LOWER MATCHES "iphone")
        find_library(FOUNDATION_FRAMEWORK Foundation)
        if(FOUNDATION_FRAMEWORK)
            target_link_libraries(CYThread PRIVATE ${FOUNDATION_FRAMEWORK})
        endif()
    else()
        find_library(COCOA_FRAMEWORK Cocoa)
        if(COCOA_FRAMEWORK)
            target_link_libraries(CYThread PRIVATE ${COCOA_FRAMEWORK})
        endif()
    endif()
elseif(ANDROID)
    target_link_libraries(CYThread PRIVATE log)
endif()

# ---- Installation (optional) -------------------------------------------------

include(GNUInstallDirs)

install(TARGETS CYThread
    EXPORT CYThreadTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CYTHREAD_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CYThread
)

install(EXPORT CYThreadTargets
    FILE CYThreadTargets.cmake
    NAMESPACE CYThread::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CYThread
)


