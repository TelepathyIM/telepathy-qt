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

    add_library(future-example-cm-call STATIC ${future_example_cm_call_SRCS})
    target_link_libraries(future-example-cm-call ${TPGLIB_LIBRARIES}
                          tp-glib-tests-future-extensions)
    tpqt4_generate_manager_file(${CMAKE_CURRENT_SOURCE_DIR}/manager-file.py example_call.manager cm.c)
endif(ENABLE_TP_GLIB_TESTS)
