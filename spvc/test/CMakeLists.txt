if(${SHADERC_ENABLE_TESTS})
  add_test(NAME spvc_spirv_cross_tests
    COMMAND ${PYTHON_EXE}
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv_cross_tests.py
    $<TARGET_FILE:spvc_exe>
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv_cross_test_inputs/test_cases
    ${SHADERC_SPIRV_CROSS_DIR})
endif()
