project(libshaderc_util)

add_library(shaderc_util STATIC
  include/libshaderc_util/file_finder.h
  include/libshaderc_util/format.h
  include/libshaderc_util/io.h
  include/libshaderc_util/resources.h
  include/libshaderc_util/string_piece.h
  include/libshaderc_util/universal_unistd.h
  src/file_finder.cc
  src/io.cc
  src/resources.cc
)

default_compile_options(shaderc_util)
target_include_directories(shaderc_util PUBLIC include PRIVATE ${glslang_SOURCE_DIR})
find_package(Threads)
target_link_libraries(shaderc_util PRIVATE
  glslang OSDependent OGLCompiler glslang ${CMAKE_THREAD_LIBS_INIT})

add_shaderc_tests(
  TEST_PREFIX shaderc_util
  LINK_LIBS shaderc_util
  TEST_NAMES
    string_piece
    format
    file_finder
    io)

# This target copies content of testdata into the build directory.
add_custom_target(testdata COMMAND
  ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/testdata/copy-to-build.cmake)

add_dependencies(shaderc_util_file_finder_test testdata)
add_dependencies(shaderc_util_io_test testdata)

add_definitions(-DCURRENT_DIR="${CMAKE_CURRENT_BINARY_DIR}")
