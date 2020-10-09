# Add the files we want to link against
set(logger_source_files
    Logger.cpp
)

# Add the library to be linked against, with the previously specified source files
add_library(Logger ${logger_source_files})
