if(${SHADERC_ENABLE_TESTS})
  add_test(NAME spvc_spirv_cross_tests
    COMMAND ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/run_spirv_cross_tests.py
    ${CMAKE_BINARY_DIR}
    ${SHADERC_SPIRV_CROSS_DIR})
endif()
