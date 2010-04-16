if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_contactlist_SRCS
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        contact-list.c
        contact-list.h
        contact-list-manager.c
        contact-list-manager.h)

    set_source_files_properties(connection-manager.c
        PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(example-cm-contactlist STATIC ${example_cm_contactlist_SRCS})
    target_link_libraries(example-cm-contactlist ${TPGLIB_LIBRARIES})

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                              ${CMAKE_CURRENT_BINARY_DIR}/_gen/example_contact_list.manager
                       COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/manager-file.py
                               ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
                               ${CMAKE_CURRENT_BINARY_DIR}/_gen)
endif(ENABLE_TP_GLIB_TESTS)
