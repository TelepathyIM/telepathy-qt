if(ENABLE_TP_GLIB_TESTS)
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(example_cm_echo_SRCS
        chan.c
        chan.h
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        im-manager.c
        im-manager.h
    )

    # make_directory is required, otherwise the command won't work!!
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/manager-file.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
            _gen)
    add_custom_target(example-cm-echo-manager DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(example-cm-echo STATIC ${example_cm_echo_SRCS})
    target_link_libraries(example-cm-echo ${TPGLIB_LIBRARIES})
    add_dependencies(example-cm-echo example-cm-echo-manager)
endif(ENABLE_TP_GLIB_TESTS)
