# generate documentation on 'make doxygen-doc'
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    find_program(QHELPGENERATOR_EXECUTABLE qhelpgenerator)
    mark_as_advanced(QHELPGENERATOR_EXECUTABLE)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(QHELPGENERATOR DEFAULT_MSG QHELPGENERATOR_EXECUTABLE)

    set(abs_top_builddir ${CMAKE_BINARY_DIR})
    set(abs_top_srcdir   ${CMAKE_SOURCE_DIR})
    set(GENERATE_HTML    YES)
    set(GENERATE_RTF     NO)
    set(GENERATE_CHM     NO)
    set(GENERATE_CHI     NO)
    set(GENERATE_LATEX   NO)
    set(GENERATE_MAN     NO)
    set(GENERATE_XML     NO)
    set(GENERATE_QHP     ${QHELPGENERATOR_FOUND})
    configure_file(doxygen.cfg.in ${CMAKE_BINARY_DIR}/doxygen.cfg)
    add_custom_target(doxygen-doc ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doxygen.cfg)
endif(DOXYGEN_FOUND)
