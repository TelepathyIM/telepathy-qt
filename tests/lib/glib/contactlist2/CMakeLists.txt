if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_contactlist2_SRCS
        conn.c
        conn.h
        connection-manager.c
        connection-manager.h
        contact-list.c
        contact-list.h
        protocol.c
        protocol.h)

    add_library(example-cm-contactlist2 STATIC ${example_cm_contactlist2_SRCS})
    target_link_libraries(example-cm-contactlist2 ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
