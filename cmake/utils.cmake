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

# Finds all transitive static library dependencies of a given target.
# This will skip libraries that were statically linked that were not
# built by CMake, for example -lpthread.
macro(get_transitive_libs target out_list)
  if (TARGET ${target})
    get_target_property(libtype ${target} TYPE)
    # If this target is a static library, get anything it depends on.
    if ("${libtype}" STREQUAL "STATIC_LIBRARY")
      get_target_property(libs ${target} LINK_LIBRARIES)
      if (libs)
        foreach(lib ${libs})
          get_transitive_libs(${lib} ${out_list})
        endforeach()
      endif()
    endif()
  endif()
  get_target_property(libname ${target} LOCATION)
  # If we know the location (i.e. if it was made with CMake) then we
  # can add it to our list.
  if (libname)
    list(INSERT ${out_list} 0 "${libname}")
  endif()
  LIST(REMOVE_DUPLICATES ${out_list})
endmacro()

# Combines the static library "target" with all of it's transitive static
# library dependencies into a single static library "new_target".
function(combine_static_lib new_target target)
  if ("${MSVC}")
    message(FATAL_ERROR "MSVC does not yet support merging static libraries")
  endif()

  set(all_libs "")
  get_transitive_libs(${target} all_libs)
  string(REPLACE ";" "\\\\naddlib " all_libs_string "${all_libs}")

  set(ar_script
    "create lib${new_target}.a\\\\naddlib ${all_libs_string}\\\\nsave\\\\nend")

  add_custom_command(OUTPUT lib${new_target}.a
    DEPENDS ${all_libs}
    COMMAND /bin/echo -e ${ar_script} | ${CMAKE_AR} -M)
  add_custom_target(${new_target}_genfile ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib${new_target}.a)

  # CMake needs to be able to see this as another normal library,
  # so import the newly created library as an imported library,
  # and set up the dependencies on the custom target.
  add_library(${new_target} STATIC IMPORTED)
  set_target_properties(${new_target}
    PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/lib${new_target}.a)
  add_dependencies(${new_target} ${new_target}_genfile)
endfunction()
