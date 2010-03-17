if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR})

    set(example_cm_callable_SRCS
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        media-channel.c
        media-channel.h
        media-manager.c
        media-manager.h
        media-stream.c
        media-stream.h)

    set_source_files_properties(connection-manager.c
        PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(example-cm-callable STATIC ${example_cm_callable_SRCS})
    target_link_libraries(example-cm-callable ${TPGLIB_LIBRARIES})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                              ${CMAKE_CURRENT_BINARY_DIR}/_gen/example_callable.manager
                       COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/manager-file.py
                                ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
                                ${CMAKE_CURRENT_BINARY_DIR}/_gen)
endif(ENABLE_TP_GLIB_TESTS)
