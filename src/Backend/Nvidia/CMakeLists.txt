# Add the files we want to link against
set(nvidia_backend_source_files
    Nvidia.cpp
    NvidiaHash.cpp
    NvidiaUtils.cu
)

add_library(NvidiaBackend ${nvidia_backend_source_files})

target_link_libraries(NvidiaBackend Nvidia)
