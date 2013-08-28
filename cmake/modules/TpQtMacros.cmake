# - Common macros for Tp-Qt

# Copyright (c) 2010, Collabora Ltd. <http://www.collabora.co.uk/>
#
# Redistribution and use is allowed according to the terms of the BSD license.

#
# These macros/functions are not exported - they are meant for internal usage into Telepathy-Qt's build system.
#
# Preamble: How dynamic generators are handled with the CMake build system.
# Telepathy-Qt strongly relies upon lots of files generated at build time through some python programs, found
# in tools/. To avoid developers the struggle of handling those manually, a set of convenience macros have been
# created to handle them with the correct dependencies. Each of those macros takes a target name as a first argument
# and creates a target with that exact name. In a similar fashion, in the last argument you can specify a list
# of targets the generated target will depend on. This way, you can handle transparently dependencies between
# generated files, while the dirty stuff is done for you in the background.
#
# macro    TPQT_EXTRACT_DEPENDS (tpqt_other tpqt_depends)
#          Internal macro used to extract arguments from ARGN
#
# function TPQT_CREATE_MOC_COMMAND_TARGET_DEPS(inputfile outputfile moc_flags moc_options target_dependencies ...)
#          This function behaves exactly like qt_create_moc_command, but creates a custom target for the
#          moc file generation, allowing to specify a list of targets the generated moc target will depend on.
#          Just like qt_create_moc_command, it is an internal macro and it's not meant to be used explicitely.
#
# function TPQT_GENERATE_MOC_I(inputfile outputfile)
#          This function behaves exactly like qt_generate_moc, but it generates moc files with the -i option,
#          which disables the generation of an #include directive. This macro has to be used always when building
#          Tp-Qt internals due to the internal header files restrictions.
#
# function TPQT_GENERATE_MOC_I_TARGET_DEPS(inputfile outputfile target_dependencies ...)
#          This function acts as an overload to QT_GENERATE_MOC_I: it does exactly the same thing, but creates a
#          custom target for the moc file generation, and adds target_dependencies to it as dependencies.
#
# function TPQT_GENERATE_MOCS(sourcefile ...)
#          Generates mocs from a list of header files. You usually want to use this function when building tests
#          or examples. Please remember the list of the header files passed to this function MUST be added to the
#          target's sources.
#
# function TPQT_CLIENT_GENERATOR(spec group pretty_include namespace [arguments] [DEPENDS dependencies ...])
#          This function takes care of invoking qt-client-gen.py with the correct arguments, which generates
#          headers out of specs. spec is the name of the spec headers will be generated from, group represents
#          the spec's group, pretty_include is the name of the capitalized header (for example ClientGenerator),
#          namespace is the C++ namespace the generated header will belong to. This function also accepts
#          as an optional last argument a list of additional command line arguments which will be passed to
#          qt-client-gen.py upon execution. After issuing DEPENDS in the last argument you can pass a list of targets
#          the generated target will depend on.
#
# function TPQT_FUTURE_CLIENT_GENERATOR(spec namespace [arguments] [DEPENDS dependencies ...])
#          Same as tpqt_client_generator, but for future interfaces
#
# function TPQT_SERVICE_GENERATOR(spec group pretty_include namespace [arguments] [DEPENDS dependencies ...])
#          This function takes care of invoking qt-svc-gen.py with the correct arguments, which generates
#          headers out of specs. spec is the name of the spec headers will be generated from, group represents
#          the spec's group, pretty_include is the name of the capitalized header (for example ServiceGenerator),
#          namespace is the C++ namespace the generated header will belong to. This function also accepts
#          as an optional last argument a list of additional command line arguments which will be passed to
#          qt-svc-gen.py upon execution. After issuing DEPENDS in the last argument you can pass a list of targets
#          the generated target will depend on.
#
# function TPQT_GENERATE_MANAGER_FILE(MANAGER_FILE OUTPUT_FILENAME DEPEND_FILENAME)
#          This function takes care of invoking manager-file.py with the correct arguments. The first argument is the
#          path to the manager-file.py file which should be used, the second is the output filename of the manager,
#          and the third is the path to the file which depends on the generated manager file.
#
# function TPQT_XINCLUDATOR (TARGET_NAME INPUT_FILE OUTPUT_FILE [additional_arguments ...] [DEPENDS dependencies ...])
#          This function takes care of invoking xincludator.py with the correct arguments. TARGET_NAME is the name of
#          the generated target (see preamble), INPUT_FILE is the input spec file, OUTPUT_FILE is the filename
#          the generated file will be saved to. This function also accepts as an optional last argument a list of
#          additional command line arguments which will be passed to xincludator upon execution.
#          After issuing DEPENDS in the last argument you can pass a list of targets the generated target will depend on.
#
# function TPQT_CONSTANTS_GEN (TARGET_NAME SPEC_XML OUTPUT_FILE [additional_arguments ...] [DEPENDS dependencies ...])
#          This function takes care of invoking qt-constants-gen.py with the correct arguments. TARGET_NAME is the name of
#          the generated target (see preamble), SPEC_XML is the spec input file, OUTPUT_FILE is the filename
#          the generated file will be saved to. This function also accepts as an optional last argument a list of
#          additional command line arguments which will be passed to qt-constants-gen.py upon execution.
#          After issuing DEPENDS in the last argument you can pass a list of targets the generated target will depend on.
#
# function TPQT_TYPES_GEN (TARGET_NAME SPEC_XML OUTFILE_DECL OUTFILE_IMPL NAMESPACE
#                           REAL_INCLUDE PRETTY_INCLUDE [additional_arguments ...] [DEPENDS dependencies ...])
#          This function takes care of invoking qt-types-gen.py with the correct arguments. TARGET_NAME is the name of
#          the generated target (see preamble), SPEC_XML is the input spec file, OUTFILE_DECL is the filename
#          the header of the generated file will be saved to, OUTFILE_IMPL is the filename the implementation of the
#          generated file will be saved to, NAMESPACE is the C++ namespace the generated header will belong to,
#          REAL_INCLUDE is the real include file you want to use, PRETTY_INCLUDE is the name of the capitalized header
#          (for example ClientGenerator).
#          This function also accepts as an optional last argument a list of additional command line arguments
#          which will be passed to qt-constants-gen.py upon execution.
#          After issuing DEPENDS in the last argument you can pass a list of targets the generated target will depend on.
#
# macro TPQT_ADD_GENERIC_UNIT_TEST (fancyName name [libraries ...])
#       This macro takes care of building and adding a generic unit test to the automatic CTest suite. The requirement
#       for using this macro is to have the unit test contained in a single source file named ${name}.cpp. fancyName will
#       be used as the test and target's name, and you can specify as a third and optional argument a set of additional
#       libraries the target will link to.
#
# macro TPQT_ADD_DBUS_UNIT_TEST (fancyName name [libraries ...])
#       This macro takes care of building and adding an unit test requiring DBus emulation to the automatic
#       CTest suite. The requirement for using this macro is to have the unit test contained in a single
#       source file named ${name}.cpp. fancyName will be used as the test and target's name, and you can specify as a third
#       and optional argument a set of additional libraries the target will link to. Please remember that you need to
#       set up the DBus environment by calling TPQT_SETUP_DBUS_TEST_ENVIRONMENT BEFORE you call this macro.
#
# macro _TPQT_ADD_CHECK_TARGETS (fancyName name command [args])
#       This is an internal macro which is meant to be used by TPQT_ADD_DBUS_UNIT_TEST and TPQT_ADD_GENERIC_UNIT_TEST.
#       It takes care of generating a check target for each test method available (currently normal execution, valgrind and
#       callgrind). This macro accepts the same arguments as the add test macros, but accepts a command and a list of
#       arguments for running the test instead of the link libraries. However, you are not meant to call this macro from
#       your CMakeLists.txt files.
#
# function TPQT_SETUP_DBUS_TEST_ENVIRONMENT ()
#          This function MUST be called before calling TPQT_ADD_DBUS_UNIT_TEST. It takes care of preparing the test
#          environment for DBus tests and generating the needed files.
#
# macro MAKE_INSTALL_PATH_ABSOLUTE (out in)
#       This macro makes the path given in the "in" variable absolute (or leaves it unchanged
#       if it's absolute already) by prefixing it with TELEPATHY_QT_INSTALL_DIR,
#       and returns the absolute path in the "out" variable. This macro is mainly used for
#       generating *Config.cmake files.
#
#

MACRO (TPQT_EXTRACT_DEPENDS _tpqt_other _tpqt_depends)
  SET(${_tpqt_other})
  SET(${_tpqt_depends})
  SET(_TPQT_DOING_DEPENDS FALSE)
  FOREACH(_currentArg ${ARGN})
    IF ("${_currentArg}" STREQUAL "DEPENDS")
      SET(_TPQT_DOING_DEPENDS TRUE)
    ELSE ("${_currentArg}" STREQUAL "DEPENDS")
      IF(_TPQT_DOING_DEPENDS)
        LIST(APPEND ${_tpqt_depends} "${_currentArg}")
      ELSE(_TPQT_DOING_DEPENDS)
        LIST(APPEND ${_tpqt_other} "${_currentArg}")
      ENDIF(_TPQT_DOING_DEPENDS)
    ENDIF ("${_currentArg}" STREQUAL "DEPENDS")
  ENDFOREACH(_currentArg)
ENDMACRO (TPQT_EXTRACT_DEPENDS)

# helper function to set up a moc rule
FUNCTION (TPQT_CREATE_MOC_COMMAND_TARGET_DEPS infile outfile moc_flags moc_options)
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
ENDFUNCTION (TPQT_CREATE_MOC_COMMAND_TARGET_DEPS)

# add the -i option to QT_GENERATE_MOC
function(TPQT_GENERATE_MOC_I infile outfile)
    qt_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    qt_create_moc_command(${abs_infile} ${outfile} "${moc_flags}" "-i")
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endfunction(TPQT_GENERATE_MOC_I)

# same as tpqt_generate_moc_i, but lets the caller specify a list of targets which the mocs should depend on
function(TPQT_GENERATE_MOC_I_TARGET_DEPS infile outfile)
    qt_get_moc_flags(moc_flags)
    get_filename_component(abs_infile ${infile} ABSOLUTE)
    tpqt_create_moc_command_target_deps(${abs_infile} ${outfile} "${moc_flags}" "-i" ${ARGN})
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC TRUE)  # dont run automoc on this file
endfunction(TPQT_GENERATE_MOC_I_TARGET_DEPS)

# generates mocs for the passed list. The list should be added to the target's sources
function(tpqt_generate_mocs)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
    foreach(moc_src ${ARGN})
        string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
        tpqt_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
        set_property(SOURCE ${moc_src} APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    endforeach(moc_src ${ARGN})
endfunction(tpqt_generate_mocs)

function(tpqt_client_generator spec group pretty_include namespace)
    tpqt_extract_depends(client_generator_args client_generator_depends ${ARGN})
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt-client-gen.py
            --group=${group}
            --namespace=${namespace}
            --typesnamespace=Tp
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
            --realinclude=TelepathyQt/${spec}.h
            --prettyinclude=TelepathyQt/${pretty_include}
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml
            --extraincludes=${TYPES_INCLUDE}
            --must-define=IN_TP_QT_HEADER
            --visibility=TP_QT_EXPORT
            ${client_generator_args})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

        DEPENDS ${CMAKE_SOURCE_DIR}/tools/libqtcodegen.py
                ${CMAKE_SOURCE_DIR}/tools/qt-client-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-${spec}.xml)
    add_custom_target(generate_cli-${spec}-body DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}-body.hpp)
    add_dependencies(all-generated-sources generate_cli-${spec}-body)

    if (client_generator_depends)
        add_dependencies(generate_cli-${spec}-body ${client_generator_depends})
    endif (client_generator_depends)

    tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-${spec}.moc.hpp
                       "generate_cli-${spec}-body")
endfunction(tpqt_client_generator spec group pretty_include namespace)

function(tpqt_future_client_generator spec namespace)
    tpqt_extract_depends(future_client_generator_args future_client_generator_depends ${ARGN})
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt-client-gen.py
            --namespace=${namespace}
            --typesnamespace=TpFuture
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
            --realinclude=TelepathyQt/future-internal.h
            --prettyinclude=TelepathyQt/future-internal.h
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml
            --extraincludes=${TYPES_INCLUDE}
            --extraincludes='<TelepathyQt/Types>'
            --extraincludes='<TelepathyQt/future-internal.h>'
            --visibility=TP_QT_NO_EXPORT
            ${future_client_generator_args})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

        DEPENDS ${CMAKE_SOURCE_DIR}/tools/libqtcodegen.py
                ${CMAKE_SOURCE_DIR}/tools/qt-client-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.xml)
    add_custom_target(generate_future-${spec}-body DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}-body.hpp)
    add_dependencies(all-generated-sources generate_future-${spec}-body)

    if (future_client_generator_depends)
        add_dependencies(generate_future-${spec}-body ${future_client_generator_depends})
    endif (future_client_generator_depends)

    tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/future-${spec}.moc.hpp
                       "generate_future-${spec}-body")
endfunction(tpqt_future_client_generator spec namespace)

function(tpqt_service_generator spec group pretty_include namespace)
    tpqt_extract_depends(service_generator_args service_generator_depends ${ARGN})
    string(REPLACE "svc-" "" spec ${spec})
    set(ARGS
        ${CMAKE_SOURCE_DIR}/tools/qt-svc-gen.py
            --group=${group}
            --namespace=${namespace}
            --typesnamespace=Tp
            --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.h
            --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.cpp
            --realinclude=TelepathyQt/_gen/svc-${spec}.h
            --mocinclude=TelepathyQt/_gen/svc-${spec}.moc.hpp
            --specxml=${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
            --ifacexml=${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-svc-${spec}.xml
            --visibility=TP_QT_EXPORT
            ${service_generator_args})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.cpp
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

        DEPENDS ${CMAKE_SOURCE_DIR}/tools/libqtcodegen.py
                ${CMAKE_SOURCE_DIR}/tools/qt-svc-gen.py
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/stable-spec.xml
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/spec-svc-${spec}.xml)
    add_custom_target(generate_service-${spec}-body DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.cpp)
    add_dependencies(all-generated-service-sources generate_service-${spec}-body)

    if (service_generator_depends)
        add_dependencies(generate_service-${spec}-body ${service_generator_depends})
    endif (service_generator_depends)

    tpqt_generate_moc_i_target_deps(${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.h
                       ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}.moc.hpp
                       "generate_service-${spec}-body")
endfunction(tpqt_service_generator spec group pretty_include namespace)

# This function is used for generating CM in various examples
function(tpqt_generate_manager_file MANAGER_FILE OUTPUT_FILENAME DEPEND_FILENAME)
    # make_directory is required, otherwise the command won't work!!
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                               ${CMAKE_CURRENT_BINARY_DIR}/_gen/${OUTPUT_FILENAME}

                       COMMAND ${PYTHON_EXECUTABLE}

                       ARGS    ${CMAKE_SOURCE_DIR}/tools/manager-file.py
                               ${MANAGER_FILE}
                               _gen

                       DEPENDS ${CMAKE_SOURCE_DIR}/tools/manager-file.py)

    set_source_files_properties(${DEPEND_FILENAME}
                                PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)
endfunction(tpqt_generate_manager_file MANAGER_FILE)

function(tpqt_xincludator _TARGET_NAME _INPUT_FILE _OUTPUT_FILE)
    tpqt_extract_depends(xincludator_gen_args xincludator_gen_depends ${ARGN})
    # Gather all .xml files in TelepathyQt and spec/ and make this target depend on those
    file(GLOB depends_xml_files ${CMAKE_SOURCE_DIR}/TelepathyQt/*.xml ${CMAKE_SOURCE_DIR}/spec/*.xml)

    add_custom_command(OUTPUT ${_OUTPUT_FILE}

                       COMMAND ${PYTHON_EXECUTABLE}

                       ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
                            ${_INPUT_FILE}
                            ${xincludator_gen_args}
                            > ${_OUTPUT_FILE}

                       DEPENDS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
                               ${_INPUT_FILE} ${depends_xml_files})
    add_custom_target(${_TARGET_NAME} DEPENDS ${_OUTPUT_FILE})

    if (xincludator_gen_depends)
        add_dependencies(${_TARGET_NAME} ${xincludator_gen_depends})
    endif (xincludator_gen_depends)
endfunction(tpqt_xincludator _TARGET_NAME _INPUT_FILE _OUTPUT_FILE)

function(tpqt_constants_gen _TARGET_NAME _SPEC_XML _OUTFILE)
    tpqt_extract_depends(constants_gen_args constants_gen_depends ${ARGN})
    # Gather all .xml files in TelepathyQt and spec/ and make this target depend on those
    file(GLOB depends_xml_files ${CMAKE_SOURCE_DIR}/TelepathyQt/*.xml ${CMAKE_SOURCE_DIR}/spec/*.xml)

    add_custom_command(OUTPUT ${_OUTFILE}

                       COMMAND ${PYTHON_EXECUTABLE}

                       ARGS    ${CMAKE_SOURCE_DIR}/tools/qt-constants-gen.py
                               ${constants_gen_args}
                               --specxml=${_SPEC_XML}
                               > ${_OUTFILE}

                       DEPENDS ${CMAKE_SOURCE_DIR}/tools/libqtcodegen.py
                               ${CMAKE_SOURCE_DIR}/tools/qt-constants-gen.py
                               ${_SPEC_XML} ${depends_xml_files})
    add_custom_target(${_TARGET_NAME} DEPENDS ${_OUTFILE})
    add_dependencies(all-generated-sources ${_TARGET_NAME})

    if (constants_gen_depends)
        add_dependencies(${_TARGET_NAME} ${constants_gen_depends})
    endif (constants_gen_depends)
endfunction (tpqt_constants_gen _TARGET_NAME _SPEC_XML _OUTFILE)

function(tpqt_types_gen _TARGET_NAME _SPEC_XML _OUTFILE_DECL _OUTFILE_IMPL _NAMESPACE _REALINCLUDE _PRETTYINCLUDE)
    tpqt_extract_depends(types_gen_args types_gen_depends ${ARGN})
    # Gather all .xml files in TelepathyQt and spec/ and make this target depend on those
    file(GLOB depends_xml_files ${CMAKE_SOURCE_DIR}/TelepathyQt/*.xml ${CMAKE_SOURCE_DIR}/spec/*.xml)

    add_custom_command(OUTPUT ${_OUTFILE_DECL} ${_OUTFILE_IMPL}
                       COMMAND ${PYTHON_EXECUTABLE}
                       ARGS ${CMAKE_SOURCE_DIR}/tools/qt-types-gen.py
                            --namespace=${_NAMESPACE}
                            --declfile=${_OUTFILE_DECL}
                            --implfile=${_OUTFILE_IMPL}
                            --realinclude=${_REALINCLUDE}
                            --prettyinclude=${_PRETTYINCLUDE}
                            ${types_gen_args}
                            --specxml=${_SPEC_XML}

                       DEPENDS ${CMAKE_SOURCE_DIR}/tools/libqtcodegen.py
                               ${CMAKE_SOURCE_DIR}/tools/qt-types-gen.py
                               ${_SPEC_XML} ${depends_xml_files})
    add_custom_target(${_TARGET_NAME} DEPENDS ${_OUTFILE_IMPL})
    add_dependencies(all-generated-sources ${_TARGET_NAME})

    if (types_gen_depends)
        add_dependencies(${_TARGET_NAME} ${types_gen_depends})
    endif (types_gen_depends)
endfunction(tpqt_types_gen _TARGET_NAME _SPEC_XML _OUTFILE_DECL _OUTFILE_IMPL _NAMESPACE _REALINCLUDE _PRETTYINCLUDE)

macro(tpqt_add_generic_unit_test _fancyName _name)
    tpqt_generate_moc_i(${_name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${_name}.cpp.moc.hpp)
    add_executable(test-${_name} ${_name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${_name}.cpp.moc.hpp)
    target_link_libraries(test-${_name} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTTEST_LIBRARY} telepathy-qt${QT_VERSION_MAJOR} tp-qt-tests ${TP_QT_EXECUTABLE_LINKER_FLAGS} ${ARGN})
    add_test(${_fancyName} ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-${_name})
    list(APPEND _telepathy_qt_test_cases test-${_name})

    # Valgrind and Callgrind targets
    _tpqt_add_check_targets(${_fancyName} ${_name} ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-${_name})
endmacro(tpqt_add_generic_unit_test _fancyName _name)

macro(tpqt_add_dbus_unit_test _fancyName _name)
    tpqt_generate_moc_i(${_name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${_name}.cpp.moc.hpp)
    add_executable(test-${_name} ${_name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${_name}.cpp.moc.hpp)
    target_link_libraries(test-${_name} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTTEST_LIBRARY} telepathy-qt${QT_VERSION_MAJOR} tp-qt-tests ${TP_QT_EXECUTABLE_LINKER_FLAGS} ${ARGN})
    set(with_session_bus ${CMAKE_CURRENT_BINARY_DIR}/runDbusTest.sh)
    add_test(${_fancyName} ${SH} ${with_session_bus} ${CMAKE_CURRENT_BINARY_DIR}/test-${_name})
    list(APPEND _telepathy_qt_test_cases test-${_name})
    list(APPEND _telepathy_qt_test_cases_dbus test-${_name})

    # Valgrind and Callgrind targets
    _tpqt_add_check_targets(${_fancyName} ${_name} ${with_session_bus} ${CMAKE_CURRENT_BINARY_DIR}/test-${_name})
endmacro(tpqt_add_dbus_unit_test _fancyName _name)

macro(_tpqt_add_check_targets _fancyName _name _runnerScript)
    set_tests_properties(${_fancyName}
        PROPERTIES
            FAIL_REGULAR_EXPRESSION "^FAIL!")

    # Standard check target
    add_custom_target(check-${_fancyName} ${SH} ${_runnerScript} ${ARGN})
    add_dependencies(check-${_fancyName} test-${_name})

    # Lcov target
    add_dependencies(lcov-check test-${_name})

    # Valgrind target
    add_custom_target(check-valgrind-${_fancyName})
    add_dependencies(check-valgrind-${_fancyName} test-${_name})

    add_custom_command(
        TARGET  check-valgrind-${_fancyName}
        COMMAND G_SLICE=always-malloc ${SH} ${_runnerScript} /usr/bin/valgrind
                --tool=memcheck
                --leak-check=full
                --leak-resolution=high
                --child-silent-after-fork=yes
                --num-callers=20
                --gen-suppressions=all
                --log-file=${CMAKE_CURRENT_BINARY_DIR}/test-${_fancyName}.memcheck.log
                --suppressions=${CMAKE_SOURCE_DIR}/tools/tp-qt-tests.supp
                --suppressions=${CMAKE_SOURCE_DIR}/tools/telepathy-glib.supp
                ${ARGN}
        WORKING_DIRECTORY
                ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running valgrind on test \"${_fancyName}\"")
    add_dependencies(check-valgrind check-valgrind-${_fancyName})

    # Callgrind target
    add_custom_target(check-callgrind-${_fancyName})
    add_dependencies(check-callgrind-${_fancyName} test-${_name})
    add_custom_command(
        TARGET  check-callgrind-${_fancyName}
        COMMAND ${SH} ${_runnerScript} /usr/bin/valgrind
                --tool=callgrind
                --dump-instr=yes
                --log-file=${CMAKE_CURRENT_BINARY_DIR}/test-${_fancyName}.callgrind.log
                --callgrind-out-file=${CMAKE_CURRENT_BINARY_DIR}/test-${_fancyName}.callgrind.out
                ${ARGN}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT
            "Running callgrind on test \"${_fancyName}\"")
    add_dependencies(check-callgrind check-callgrind-${_fancyName})
endmacro(_tpqt_add_check_targets _fancyName _name)

function(tpqt_setup_dbus_test_environment)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runDbusTest.sh "
${test_environment}
sh ${CMAKE_SOURCE_DIR}/tools/with-session-bus.sh \\
    --config-file=${CMAKE_BINARY_DIR}/tests/dbus-1/session.conf -- $@
")
endfunction(tpqt_setup_dbus_test_environment)

macro(make_install_path_absolute out in)
    if (IS_ABSOLUTE "${in}")
        set(${out} "${in}")
    else (IS_ABSOLUTE "${in}")
        set(${out} "\${TELEPATHY_QT${QT_VERSION_MAJOR}_INSTALL_DIR}/${in}")
    endif (IS_ABSOLUTE "${in}")
endmacro(make_install_path_absolute out in)
