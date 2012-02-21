if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_call_SRCS
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
        conn.h
        protocol.c
        protocol.h)

    add_library(example-cm-call STATIC ${example_cm_call_SRCS})
    target_link_libraries(example-cm-call ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
