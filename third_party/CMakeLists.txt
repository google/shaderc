# Suppress all warnings from third-party projects.
set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)

# Configure third party projects.
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/googletest)
  add_subdirectory(googletest)
endif()
if (NOT TARGET gmock)
  message(FATAL_ERROR "gmock was not found - required for tests")
endif()

set(OLD_PLATFORM_TOOLSET ${CMAKE_GENERATOR_TOOLSET})

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
  add_subdirectory(glslang)
endif()
if (NOT TARGET glslang)
  message(FATAL_ERROR "glslang was not found - required for compilation")
endif()
if(WIN32)
    # This is unfortunate but glslang forces our
    # platform toolset to be v110, which we may not even have
    # installed, undo anything glslang has done to it.
    set(CMAKE_GENERATOR_TOOLSET "${OLD_PLATFORM_TOOLSET}" CACHE STRING
      "Platform Toolset" FORCE)
endif()

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spirv-tools)
  add_subdirectory(spirv-tools)
endif()
if (NOT TARGET SPIRV-Tools)
  message(FATAL_ERROR "SPIRV-Tools was not found - required for compilation")
endif()

# Configure out-of-source-directory tests for glslang.
# The glslang project uses a bash script called "runtests" to run tests.
# The runtests script assumes the glslangValidator executable exists in
# a location inside the source tree, but we build it elsewhere.
# We need to copy the test files, fix the path references, and then run tests.
set(GLSLANG_TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glslang/Test)
set(GLSLANG_TEST_BIN_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/${CMAKE_CFG_INTDIR})

# If we are building in a multi-configuration setting we have
# to put the glslang tests into their respective subdirectories.
if (CMAKE_CONFIGURATION_TYPES)
  set(GLSLANG_CONFIGURATION_DIR ${CMAKE_CFG_INTDIR})
endif()

add_custom_target(copy-tests-if-necessary ALL
  COMMAND ${PYTHON_EXE} ${shaderc_SOURCE_DIR}/utils/copy-tests-if-necessary.py
    ${GLSLANG_TEST_SRC_DIR} ${GLSLANG_TEST_BIN_DIR} ${GLSLANG_CONFIGURATION_DIR}
  COMMENT "Copying and patching glslang tests if needed")

if (CMAKE_CONFIGURATION_TYPES)
  # If we are running a multi-configuration project,
  # the tests will be in test-glslang/${Configuration}
  add_test(NAME glslang-testsuite
    COMMAND bash runtests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/$<CONFIGURATION>
  )
else()
  add_test(NAME glslang-testsuite
    COMMAND bash runtests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test-glslang/
  )
endif()

add_test(NAME spirv-tools-testsuite
  COMMAND UnitSPIRV
  WORKING_DIRECTORY ${SPIRV-Tools_BINARY_DIR})
