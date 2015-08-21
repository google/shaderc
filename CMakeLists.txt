cmake_minimum_required(VERSION 2.8.12)
project(shaderc)
enable_testing()

message(STATUS "Shaderc: build type is \"${CMAKE_BUILD_TYPE}\".")

include(cmake/setup_build.cmake)
include(cmake/utils.cmake)

add_custom_target(check-copyright ALL
  ${PYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/add_copyright.py --check
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Check copyright")

add_custom_target(add-copyright
  ${PYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/add_copyright.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Add copyright")

# Configure subdirectories.
# We depend on these for later projects, so they should come first.
add_subdirectory(third_party)

add_subdirectory(libshaderc_util)
add_subdirectory(libshaderc)
add_subdirectory(glslc)
