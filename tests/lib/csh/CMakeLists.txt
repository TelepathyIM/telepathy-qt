if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR})

    set(tp_glib_csh_tests_SRCS
        conn.c
        conn.h
        room.c
        room.h
        room-manager.c
        room-manager.h)

    add_library(tp-glib-csh-tests SHARED ${tp_glib_csh_tests_SRCS})
    target_link_libraries(tp-glib-csh-tests ${TPGLIB_LIBRARIES})
endif(ENABLE_TP_GLIB_TESTS)
