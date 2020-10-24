# We need 3.8 for set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.8)

project(violetminer LANGUAGES CXX)

option(ENABLE_NVIDIA "Enable Nvidia support" ON)
option(ENABLE_SSL "Enable SSL support if available" ON)
option(NVIDIA_ARCH "Nvidia architectures to compile for" "20 21 30 32 35 37 50 52 53 60 61 62 70 72 75 80 86")

if (ENABLE_NVIDIA)
    enable_language(CUDA)
    add_definitions(-DNVIDIA_ENABLED)
else()
    message(STATUS "NVIDIA support disabled!")
endif()

# Assert our compiler is good
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC 7.0 or higher
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
        message(FATAL_ERROR "GCC/G++ 7.0 or greater is required to compile. Please check the README.md for detailed compilation instructions.")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang 6.0 or higher
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
        message(FATAL_ERROR "Clang 6.0 or greater is required to compile. Please check the README.md for detailed compilation instructions.")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Visual Studio 15 2017 or higher
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.10)
        message(FATAL_ERROR "MSVC 19.10 or greater is required to compile (Latest Visual Studio 15 2017 should suffice). Please check the README.md for detailed compilation instructions.")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
else()
    message(WARNING "You are using an unsupported compiler. The compilation is likely to fail. God speed.")
endif()

# Enable c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Pop binaries in the root build folder, not nested
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Set optimization on by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(FORCE_ARCH "" CACHE STRING "Force the architecture to the specific string")

# Add custom modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/Argon2/src/Intrinsics")

# Use the TargetArch module to figure out what arch we're compiling for
include(TargetArch OPTIONAL RESULT_VARIABLE FOUND_TARGET_ARCH)

if("${FOUND_TARGET_ARCH}" STREQUAL "NOTFOUND")
    message(FATAL_ERROR "It looks like you haven't fetched the git submodules. Please run \'git submodule update --init --recursive\' and read the build guide carefully.")
endif()

target_architecture(ARCH)

if("${ARCH}" STREQUAL "x86_64")
    add_definitions(-DX86_OPTIMIZATIONS)
elseif("${ARCH}" STREQUAL "armv8")
    add_definitions(-DARM_OPTIMIZATIONS)
    add_definitions(-DARMV8_OPTIMIZATIONS)
elseif("${ARCH}" STREQUAL "armv7")
    add_definitions(-DARM_OPTIMIZATIONS)
    add_definitions(-DARMV7_OPTIMIZATIONS)
elseif("${ARCH}" STREQUAL "arm")
    add_definitions(-DARM_OPTIMIZATIONS)
else()
    add_definitions(-DNO_OPTIMIZATIONS)
endif()

set(OPENSSL_USE_STATIC_LIBS ON)

if (MSVC)
    set(OPENSSL_MSVC_STATIC_RT ON)
endif ()

# We have to look for Homebrew OpenSSL a bit differently
# Borrowed from https://github.com/tarantool/tarantool/commit/6eab201af1843f53a833c8928dc58fceffa08147
if (APPLE AND ENABLE_SSL)
    find_program(HOMEBREW_EXECUTABLE brew)
    execute_process(COMMAND ${HOMEBREW_EXECUTABLE} --prefix openssl
            OUTPUT_VARIABLE HOMEBREW_OPENSSL
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (DEFINED HOMEBREW_OPENSSL)
        if (NOT DEFINED OPENSSL_ROOT_DIR)
            message(STATUS "Setting OpenSSL root to ${HOMEBREW_OPENSSL}")
            set(OPENSSL_ROOT_DIR "${HOMEBREW_OPENSSL}")
        endif ()
    endif ()
endif ()

# Try and find OpenSSL
if (ENABLE_SSL)
    find_package(OpenSSL)
endif()

# Didn't work. Lets try finding it with PkgConfig instead
if (NOT OPENSSL_FOUND AND ENABLE_SSL)
    find_package(PkgConfig)

    if (PKG_CONFIG_FOUND)
        pkg_search_module(OPENSSL openssl)
    endif()
endif()

if (OPENSSL_FOUND)
    # On non MSVC build systems, we need to link ldl with the static OpenSSL library
    if (NOT MSVC)
        set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES};dl")
    endif ()

    include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})

    # Enable SSL support with our SocketWrapper
    add_definitions(-DSOCKETWRAPPER_OPENSSL_SUPPORT)

    message(STATUS "OpenSSL Found: ${OPENSSL_INCLUDE_DIR}")
elseif(ENABLE_SSL)
    message(STATUS "OpenSSL Found: No... Skipping...")
else()
    message(STATUS "OpenSSL disabled: Not searching.")
endif ()

add_subdirectory(src)

if (ENABLE_NVIDIA)
    list(APPEND NVIDIA_TARGETS violetminer NvidiaBackend Nvidia)

    foreach(target ${NVIDIA_TARGETS})
        target_compile_features(${target} PUBLIC cxx_std_14)
    endforeach()

    set_target_properties(${NVIDIA_TARGETS}
                          PROPERTIES
                          CUDA_ARCHITECTURES "${NVIDIA_ARCH}"
                          CUDA_SEPARABLE_COMPILATION ON
                          CUDA_STANDARD 14)

    if (MSVC)
        set_target_properties(${NVIDIA_TARGETS}
                              PROPERTIES
                              CUDA_RESOLVE_DEVICE_SYMBOLS ON)
    endif()

    # Makes cuda-memcheck, cuda-gdb, etc, much more helpful
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -g -G")
    else()
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")
    endif()
endif()
