find_package(SWIG)
find_package(PythonLibs)
if(NOT SWIG_FOUND OR NOT PYTHONLIBS_FOUND)
    return()
endif()
include(GrSwig)
include(GrPython)

foreach(incdir ${GNURADIO_CORE_INCLUDE_DIRS})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/swig)
endforeach(incdir)

foreach(incdir ${GRUEL_INCLUDE_DIRS})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/gruel/swig)
endforeach(incdir)

set(GR_SWIG_LIBRARIES op25)
set(GR_SWIG_DOC_FILE ${CMAKE_CURRENT_BINARY_DIR}/op25_swig_doc.i)
set(GR_SWIG_DOC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include)

GR_SWIG_MAKE(op25_swig op25_swig.i)

GR_SWIG_INSTALL(TARGETS op25_swig DESTINATION ${GR_PYTHON_DIR}/op25)

install(
    FILES
    op25_swig.i
    ${CMAKE_CURRENT_BINARY_DIR}/op25_swig_doc.i
    DESTINATION ${GR_INCLUDE_DIR}/op25/swig
)
