find_package(Threads)

add_library(glslc STATIC
  src/file_compiler.cc
  src/file_compiler.h
  src/file.cc
  src/file.h
  src/file_includer.cc
  src/file_includer.h
  src/shader_stage.cc
  src/shader_stage.h
)

default_compile_options(glslc)
target_include_directories(glslc PUBLIC ${glslang_SOURCE_DIR})
target_link_libraries(glslc PRIVATE glslang OSDependent OGLCompiler
  glslang SPIRV ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(glslc PRIVATE shaderc_util)

add_executable(glslc_exe src/main.cc)
default_compile_options(glslc_exe)
set_target_properties(glslc_exe PROPERTIES OUTPUT_NAME glslc)
target_link_libraries(glslc_exe PRIVATE glslc shaderc_util)

add_shaderc_tests(
  TEST_PREFIX glslc
  LINK_LIBS glslc shaderc_util
  TEST_NAMES
    file
    stage)

add_asciidoc(glslc_doc_README README)

add_subdirectory(test)
