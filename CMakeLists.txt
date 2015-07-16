cmake_minimum_required(VERSION 2.8.12)
project(shaderc)
enable_testing()

include(cmake/setup_build.cmake)
include(cmake/utils.cmake)

# Configure subdirectories.
# We depend on these for later projects, so they should come first.
add_subdirectory(third_party)

add_subdirectory(libshaderc_util)
add_subdirectory(libshaderc)
add_subdirectory(glslc)


