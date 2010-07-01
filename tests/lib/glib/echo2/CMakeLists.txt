if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_echo2_SRCS
        chan.c
        chan.h
        conn.c
        connection-manager.c
        connection-manager.h
        conn.h
        im-manager.c
        im-manager.h)

    add_library(example-cm-echo2 STATIC ${example_cm_echo2_SRCS})
    target_link_libraries(example-cm-echo2 ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
