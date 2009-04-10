if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR})

    set(example_cm_echo_SRCS
        chan.c
        chan.h
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        im-manager.c
        im-manager.h)

    add_library(tp-glib-example-cm-echo SHARED ${example_cm_echo_SRCS})
    target_link_libraries(tp-glib-example-cm-echo ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
