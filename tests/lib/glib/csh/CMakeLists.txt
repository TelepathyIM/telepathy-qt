if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_csh_SRCS
        conn.c
        conn.h
        room.c
        room.h
        room-manager.c
        room-manager.h)

    add_library(example-cm-csh STATIC ${example_cm_csh_SRCS})
    target_link_libraries(example-cm-csh ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
