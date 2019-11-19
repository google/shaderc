add_test(NAME shaderc_expect_unittests
         COMMAND ${PYTHON_EXECUTABLE} -m unittest expect_unittest.py
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME shaderc_glslc_test_framework_unittests
         COMMAND ${PYTHON_EXECUTABLE} -m unittest glslc_test_framework_unittest.py
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(${SHADERC_ENABLE_TESTS})
  add_test(NAME glslc_tests
    COMMAND ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/glslc_test_framework.py
    $<TARGET_FILE:glslc_exe> $<TARGET_FILE:spirv-dis>
    --test-dir ${CMAKE_CURRENT_SOURCE_DIR})
endif()
