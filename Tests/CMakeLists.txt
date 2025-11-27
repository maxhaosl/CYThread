cmake_minimum_required(VERSION 3.16)
project(CYThreadTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the CYThread library
set(CYTHREAD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Include directories
include_directories(${CYTHREAD_ROOT}/Inc)
include_directories(${CYTHREAD_ROOT}/Src)

# Find the built library
if(WIN32)
    set(CYTHREAD_LIB_DIR ${CYTHREAD_ROOT}/Bin/Windows/x64/Debug)
    set(CYTHREAD_LIB_NAME CYThread)
else()
    set(CYTHREAD_LIB_DIR ${CYTHREAD_ROOT}/Build/out/macos/shared/Debug)
    set(CYTHREAD_LIB_NAME CYThread)
endif()

# Create the test executable
add_executable(CYThreadTest TestSample.cpp)

# Link against the CYThread library
target_link_libraries(CYThreadTest ${CYTHREAD_LIB_DIR}/lib${CYTHREAD_LIB_NAME}.dylib)

# Platform-specific settings
if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    target_link_libraries(CYThreadTest ${FOUNDATION_FRAMEWORK})
endif()

# Find and link threads
find_package(Threads REQUIRED)
target_link_libraries(CYThreadTest Threads::Threads)

# Set output directory
set_target_properties(CYThreadTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

# Copy the library to the test directory for runtime
if(APPLE)
    add_custom_command(TARGET CYThreadTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CYTHREAD_LIB_DIR}/libCYThread.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/libCYThread.dylib
    )
endif()
