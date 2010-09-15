if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_csh_SRCS
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        room.c
        room.h
        room-manager.c
        room-manager.h)

    # make_directory is required, otherwise the command won't work!!
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/manager-file.py
            ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
            _gen)
    add_custom_target(example-cm-csh-manager DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(example-cm-csh STATIC ${example_cm_csh_SRCS})
    target_link_libraries(example-cm-csh ${TPGLIB_LIBRARIES})
    add_dependencies(example-cm-csh example-cm-csh-manager)
endif(ENABLE_TP_GLIB_TESTS)
