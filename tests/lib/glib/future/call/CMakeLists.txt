if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}/tests/lib/glib/future)

    set(future_example_cm_call_SRCS
        call-channel.c
        call-channel.h
        call-content.c
        call-content.h
        call-manager.c
        call-manager.h
        call-stream.c
        call-stream.h
        cm.c
        cm.h
        conn.c
        conn.h)

    set_source_files_properties(cm.c
        PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h)

    add_library(future-example-cm-call STATIC ${future_example_cm_call_SRCS})
    target_link_libraries(future-example-cm-call ${TPGLIB_LIBRARIES}
                          tp-glib-tests-future-extensions)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/param-spec-struct.h
                              ${CMAKE_CURRENT_BINARY_DIR}/_gen/example_call.manager
                       COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/manager-file.py
                                ${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py
                                ${CMAKE_CURRENT_BINARY_DIR}/_gen)
endif(ENABLE_TP_GLIB_TESTS)
