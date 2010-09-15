# - Common macros for Tp-Qt4

# Copyright (c) 2010, Collabora Ltd. <http://www.collabora.co.uk/>
#
# Redistribution and use is allowed according to the terms of the BSD license.

# helper function to set up a moc rule
FUNCTION (QT4_CREATE_MOC_COMMAND_TARGET_DEPS infile outfile moc_flags moc_options)
  # For Windows, create a parameters file to work around command line length limit
  GET_FILENAME_COMPONENT(_moc_outfile_name "${outfile}" NAME)
  IF (WIN32)
    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    GET_FILENAME_COMPONENT(_moc_outfile_dir "${outfile}" PATH)
    IF(_moc_outfile_dir)
      SET(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    ENDIF(_moc_outfile_dir)
    SET (_moc_parameters_file ${outfile}_parameters)
    SET (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    FILE (REMOVE ${_moc_parameters_file})
    FOREACH(arg ${_moc_parameters})
      FILE (APPEND ${_moc_parameters_file} "${arg}\n")
    ENDFOREACH(arg)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE} @${_moc_outfile_name}_parameters
                       DEPENDS ${infile}
                       ${_moc_working_dir}
                       VERBATIM)
  ELSE (WIN32)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                       COMMAND ${QT_MOC_EXECUTABLE}
                       ARGS ${moc_flags} ${moc_options} -o ${outfile} ${infile}
                       DEPENDS ${infile})
  ENDIF (WIN32)

  add_custom_target(moc-${_moc_outfile_name} DEPENDS ${outfile})
  add_dependencies(moc-${_moc_outfile_name} ${ARGN})
ENDFUNCTION (QT4_CREATE_MOC_COMMAND_TARGET_DEPS)

# add the -i option to QT4_GENERATE_MOC
function(QT4_GENERATE_MOC_I infile outfile)
    qt4_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt4_create_moc_command(${abs_infile} ${outfile} "${moc_flags}" "-i")
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endfunction(QT4_GENERATE_MOC_I)

# same as qt4_generate_moc_i, but lets the caller specify a list of targets which the mocs should depend on
function(QT4_GENERATE_MOC_I_TARGET_DEPS infile outfile)
    qt4_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt4_create_moc_command_target_deps(${abs_infile} ${outfile} "${moc_flags}" "-i" ${ARGN})
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endfunction(QT4_GENERATE_MOC_I_TARGET_DEPS)

# generates mocs for the passed list. The list should be added to the target's sources
function(tpqt4_generate_mocs)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
    foreach(moc_src ${ARGN})
        string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
        qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
        macro_add_file_dependencies(${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    endforeach(moc_src ${ARGN})
endfunction(tpqt4_generate_mocs)

function(tpqt4_client_generator spec group pretty_include namespace main_iface)
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
            --group=${group}
            --namespace=${namespace}
            --typesnamespace=Tp
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
            --realinclude=TelepathyQt4/${spec}.h
            --prettyinclude=TelepathyQt4/${pretty_include}
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
            --extraincludes=${TYPES_INCLUDE}
            --must-define=IN_TELEPATHY_QT4_HEADER
            --visibility=TELEPATHY_QT4_EXPORT
            ${main_iface})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        DEPENDS ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # Tell CMake the source won't be available until build time.
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp PROPERTIES GENERATED 1)
    add_custom_target(generate_cli-${spec}-body DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp)
    qt4_generate_moc_i_target_deps(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp
                       "generate_cli-${spec}-body")
    # Tell CMake the source won't be available until build time.
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp PROPERTIES GENERATED 1)
    list(APPEND telepathy_qt4_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp)
endfunction(tpqt4_client_generator spec group pretty_include main_iface)

function(tpqt4_future_client_generator spec namespace main_iface)
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
            --namespace=${namespace}
            --typesnamespace=TpFuture
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
            --realinclude=TelepathyQt4/future-internal.h
            --prettyinclude=TelepathyQt4/future-internal.h
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml
            --extraincludes=${TYPES_INCLUDE}
            --extraincludes='<TelepathyQt4/Types>'
            --extraincludes='<TelepathyQt4/future-internal.h>'
            --visibility=TELEPATHY_QT4_NO_EXPORT
            ${main_iface})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        DEPENDS ${CMAKE_SOURCE_DIR}/tools/qt4-client-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    # Tell CMake the source won't be available until build time.
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp PROPERTIES GENERATED 1)
    add_custom_target(generate_future-${spec}-body DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp)
    qt4_generate_moc_i_target_deps(${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.moc.hpp
                       "generate_future-${spec}-body")
    # Tell CMake the source won't be available until build time.
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.moc.hpp PROPERTIES GENERATED 1)
endfunction(tpqt4_future_client_generator spec group pretty_include main_iface)

function(tpqt4_add_generic_unit_test fancyName name)
    qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    add_executable(test-${name} ${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    target_link_libraries(test-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests)
    add_test(${fancyName} ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-${name})
endfunction(tpqt4_add_generic_unit_test fancyName name)

function(tpqt4_add_dbus_unit_test fancyName name libraries)
    qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    add_executable(test-${name} ${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    target_link_libraries(test-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests ${libraries})
    add_test(${fancyName} ${with_session_bus} ${CMAKE_CURRENT_BINARY_DIR}/test-${name})
endfunction(tpqt4_add_dbus_unit_test fancyName name libraries)

# This function is used for generating CM in various examples
function(tpqt4_generate_manager_file MANAGER_FILE OUTPUT_FILENAME DEPEND_FILENAME)
    # make_directory is required, otherwise the command won't work!!
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                               ${CMAKE_CURRENT_BINARY_DIR}/_gen/${OUTPUT_FILENAME}

                       COMMAND ${PYTHON_EXECUTABLE}

                       ARGS    ${CMAKE_SOURCE_DIR}/tools/manager-file.py
                               ${MANAGER_FILE}
                               _gen)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                                ${CMAKE_CURRENT_BINARY_DIR}/_gen/${OUTPUT_FILENAME}
                                PROPERTIES GENERATED true)
    set_source_files_properties(${DEPEND_FILENAME}
                                PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)
endfunction(tpqt4_generate_manager_file MANAGER_FILE)

function(tpqt4_xincludator _INPUT_FILE _OUTPUT_FILE)
    add_custom_command(OUTPUT ${_OUTPUT_FILE}

                       COMMAND ${PYTHON_EXECUTABLE}

                       ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
                            ${_INPUT_FILE}
                            > ${_OUTPUT_FILE}

                       DEPENDS ${_INPUT_FILE}
                               ${CMAKE_SOURCE_DIR}/tools/xincludator.py
                               ${ARGN})

    set_source_files_properties(${_OUTPUT_FILE} PROPERTIES GENERATED true)
endfunction(tpqt4_xincludator _INPUT_FILE _OUTPUT_FILE)

function(tpqt4_constants_gen _SPEC_XML _OUTFILE)
    add_custom_command(OUTPUT ${_OUTFILE}
                        COMMAND ${PYTHON_EXECUTABLE}
                        ARGS ${CMAKE_SOURCE_DIR}/tools/qt4-constants-gen.py
                            ${ARGN}
                            --specxml=${_SPEC_XML}
                            > ${_OUTFILE}
                        DEPENDS ${_SPEC_XML}
                                ${CMAKE_SOURCE_DIR}/tools/qt4-constants-gen.py)

    set_source_files_properties(${_OUTFILE} PROPERTIES GENERATED true)
endfunction (tpqt4_constants_gen _SPEC_XML _OUTFILE)

function(tpqt4_types_gen _SPEC_XML _OTHER_DEP _OUTFILE_DECL _OUTFILE_IMPL _NAMESPACE _REALINCLUDE _PRETTYINCLUDE)
    add_custom_command(OUTPUT ${_OUTFILE_DECL} ${_OUTFILE_IMPL}
                       COMMAND ${PYTHON_EXECUTABLE}
                       ARGS ${CMAKE_SOURCE_DIR}/tools/qt4-types-gen.py
                            --namespace=${_NAMESPACE}
                            --declfile=${_OUTFILE_DECL}
                            --implfile=${_OUTFILE_IMPL}
                            --realinclude=${_REALINCLUDE}
                            --prettyinclude=${_PRETTYINCLUDE}
                            ${ARGN}
                            --specxml=${_SPEC_XML}
                        DEPENDS ${_SPEC_XML}
                                ${CMAKE_SOURCE_DIR}/tools/qt4-types-gen.py
                                ${_OTHER_DEP})
    set_source_files_properties(${_OUTFILE_DECL} PROPERTIES GENERATED true)
    set_source_files_properties(${_OUTFILE_IMPL} PROPERTIES GENERATED true)
endfunction(tpqt4_types_gen _SPEC_XML _OTHER_DEP _OUTFILE_DECL _OUTFILE_IMPL _NAMESPACE _REALINCLUDE _PRETTYINCLUDE)
