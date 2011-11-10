if(ENABLE_TP_GLIB_TESTS)
    set(example_cm_echo2_SRCS
        chan.c
        chan.h
        conn.c
        connection-manager.c
        connection-manager.h
        conn.h
        im-manager.c
        im-manager.h
        protocol.c
        protocol.h)

    add_library(example-cm-echo2 STATIC ${example_cm_echo2_SRCS})
    target_link_libraries(example-cm-echo2 ${TPGLIB_LIBRARIES})
    tpqt_generate_manager_file(${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py example_echo_2.manager connection-manager.c)
endif(ENABLE_TP_GLIB_TESTS)
