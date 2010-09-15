include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${TELEPATHY_GLIB_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${GOBJECT_INCLUDE_DIR}
    ${DBUS_INCLUDE_DIR})

if(ENABLE_TP_GLIB_TESTS)
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


    add_subdirectory(callable)
    add_subdirectory(contactlist)
    add_subdirectory(csh)
    add_subdirectory(echo)
    add_subdirectory(echo2)
    add_subdirectory(future)

    set(tp_glib_tests_SRCS
        contacts-conn.c
        contacts-conn.h
        debug.h
        params-cm.c
        params-cm.h
        simple-account.c
        simple-account.h
        simple-account-manager.c
        simple-account-manager.h
        simple-channel-dispatch-operation.c
        simple-channel-dispatch-operation.h
        simple-client.c
        simple-client.h
        simple-conn.c
        simple-conn.h
        simple-manager.c
        simple-manager.h
        textchan-group.c
        textchan-group.h
        textchan-null.c
        textchan-null.h
        util.c
        util.h)
    add_library(tp-glib-tests SHARED ${tp_glib_tests_SRCS})
    target_link_libraries(tp-glib-tests ${TELEPATHY_GLIB_LIBRARIES}
                          example-cm-callable
                          example-cm-contactlist
                          example-cm-csh
                          example-cm-echo
                          example-cm-echo2
                          tp-glib-tests-future-extensions
                          future-example-cm-call
                          future-example-cm-conference)

endif(ENABLE_TP_GLIB_TESTS)
