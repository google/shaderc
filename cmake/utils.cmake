# utility functions

function (use_gmock TARGET)
  target_include_directories(${TARGET} PRIVATE
    ${gmock_SOURCE_DIR}/include
    ${gtest_SOURCE_DIR}/include)
  target_link_libraries(${TARGET} PRIVATE gmock gtest_main)
endfunction(use_gmock)

function(default_compile_options TARGET)
  if (NOT "${MSVC}")
    target_compile_options(${TARGET} PRIVATE -std=c++11 -fPIC -Wall -Werror)
    if (ENABLE_CODE_COVERAGE)
      # The --coverage option is a synonym for -fprofile-arcs -ftest-coverage
      # when compiling.
      target_compile_options(${TARGET} PRIVATE -g -O0 --coverage)
      # The --coverage option is a synonym for -lgcov when linking for gcc.
      # For clang, it links in a different library, libclang_rt.profile, which
      # requires clang to be built with compiler-rt.
      target_link_libraries(${TARGET} PRIVATE --coverage)
    endif()
  else()
    # disable warning C4800: 'int' : forcing value to bool 'true' or 'false'
    # (performance warning)
    target_compile_options(${TARGET} PRIVATE /wd4800)
  endif()
endfunction(default_compile_options)

# Build an asciidoc file; additional arguments past the base filename specify
# additional dependencies for the file.
function(add_asciidoc TARGET FILE)
  if (ASCIIDOCTOR_EXE)
    set(DEST ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.html)
    add_custom_command(
      COMMAND ${ASCIIDOCTOR_EXE} -a toc -o ${DEST}
        ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.asciidoc
      DEPENDS ${FILE}.asciidoc ${ARGN}
      OUTPUT ${DEST})
    add_custom_target(${TARGET} ALL DEPENDS ${DEST})
  endif(ASCIIDOCTOR_EXE)
endfunction()

# Run nosetests in the current directory. Nosetests will look for python files
# with "nosetest" in their name, as well as descending into directories with
# "nosetest" in their name. The test name will be ${PREFIX}_nosetests.
function(add_nosetests PREFIX)
  if(NOSETESTS_EXE)
    add_test(
      NAME ${PREFIX}_nosetests
      COMMAND ${NOSETESTS_EXE}
        -m "(?:^|[b_./-])[Nn]ose[Tt]est" -v ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endfunction()

# Adds a set of tests.
# This function accepts the following parameters:
# TEST_PREFIX:  a prefix for each test target name
# TEST_NAMES:   a list of test names where each TEST_NAME has a corresponding
#               file residing at src/${TEST_NAME}_test.cc
# LINK_LIBS:    (optional) a list of libraries to be linked to the test target
# INCLUDE_DIRS: (optional) a list of include directories to be searched
#               for header files.
function(add_shaderc_tests)
  cmake_parse_arguments(PARSED_ARGS
    ""
    "TEST_PREFIX"
    "TEST_NAMES;LINK_LIBS;INCLUDE_DIRS"
    ${ARGN})
  if (NOT PARSED_ARGS_TEST_NAMES)
    message(FATAL_ERROR "Tests must have a target")
  endif()
  if (NOT PARSED_ARGS_TEST_PREFIX)
    message(FATAL_ERROR "Tests must have a prefix")
  endif()
  foreach(TARGET ${PARSED_ARGS_TEST_NAMES})
    set(TEST_NAME ${PARSED_ARGS_TEST_PREFIX}_${TARGET}_test)
    add_executable(${TEST_NAME} src/${TARGET}_test.cc)
    default_compile_options(${TEST_NAME})
    if (PARSED_ARGS_LINK_LIBS)
      target_link_libraries(${TEST_NAME} PRIVATE
        ${PARSED_ARGS_LINK_LIBS})
    endif()
    if (PARSED_ARGS_INCLUDE_DIRS)
      target_include_directories(${TEST_NAME} PRIVATE
        ${PARSED_ARGS_INCLUDE_DIRS})
    endif()
    use_gmock(${TEST_NAME})
    add_test(
      NAME ${PARSED_ARGS_TEST_PREFIX}_${TARGET}
      COMMAND ${TEST_NAME})
  endforeach()
endfunction(add_shaderc_tests)
