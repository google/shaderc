project(libshaderc)

add_library(shaderc STATIC
  include/shaderc.h
  src/shaderc.cc
  src/shaderc_private.h
)

default_compile_options(shaderc)
target_include_directories(shaderc PUBLIC include PRIVATE ${glslang_SOURCE_DIR})
find_package(Threads)
target_link_libraries(shaderc PRIVATE
  glslang OSDependent OGLCompiler glslang ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(shaderc PRIVATE shaderc_util)
target_link_libraries(shaderc PRIVATE SPIRV)


add_shaderc_tests(
  TEST_PREFIX shaderc
  LINK_LIBS shaderc
  INCLUDE_DIRS include ${glslang_SOURCE_DIR}
  TEST_NAMES
    shaderc
    shaderc_cpp)


if (NOT "${MSVC}")
  # If we are not on windows, then create a single static library
  # that contains everything needed to use shaderc.
  combine_static_lib(shaderc_combined shaderc)
  add_shaderc_tests(
    TEST_PREFIX shaderc_combined
    LINK_LIBS shaderc_combined ${CMAKE_THREAD_LIBS_INIT}
    INCLUDE_DIRS include ${glslang_SOURCE_DIR}
    TEST_NAMES
      shaderc
      shaderc_cpp)
endif()

