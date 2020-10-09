include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Argon2/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if (ENABLE_NVIDIA)
    # Only include directories if any were found
    if (NOT "${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}" STREQUAL "")
        include_directories("${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
    endif()
endif()

add_subdirectory(ArgonVariants)

add_subdirectory(Argon2)

add_subdirectory(Backend)

add_subdirectory(Config)

add_subdirectory(Logger)

add_subdirectory(Miner)

add_subdirectory(MinerManager)

add_subdirectory(PoolCommunication)

add_subdirectory(Types)

add_subdirectory(Utilities)
