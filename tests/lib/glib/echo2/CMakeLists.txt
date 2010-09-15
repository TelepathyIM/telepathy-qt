if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_echo2_SRCS
        chan.c
        chan.h
        conn.c
        connection-manager.c
        connection-manager.h
        conn.h
        im-manager.c
        im-manager.h
        protocol.c
        protocol.h)

    # make_directory is required, otherwise the command won't work!!
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/manager-file.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
            _gen)
    add_custom_target(example-cm-echo2-manager DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(example-cm-echo2 STATIC ${example_cm_echo2_SRCS})
    target_link_libraries(example-cm-echo2 ${TPGLIB_LIBRARIES})
    add_dependencies(example-cm-echo2 example-cm-echo2-manager)
endif(ENABLE_TP_GLIB_TESTS)
