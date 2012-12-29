include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()

GR_PYTHON_INSTALL(
    FILES
    __init__.py
    DESTINATION ${GR_PYTHON_DIR}/op25
)

include(GrTest)

set(GR_TEST_TARGET_DEPS op25)
set(GR_TEST_PYTHON_DIRS ${CMAKE_BINARY_DIR}/swig)
