if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}/tests/lib/glib
        ${CMAKE_SOURCE_DIR}/tests/lib/glib/future
        ${DBUS_GLIB_INCLUDE_DIRS})

    set(future_example_conn_addressing_SRCS
        conn.c
        conn.h)

    add_library(future-example-conn-addressing STATIC ${future_example_conn_addressing_SRCS})
    target_link_libraries(future-example-conn-addressing ${TPGLIB_LIBRARIES}
                          tp-glib-tests
                          tp-glib-tests-future-extensions)
endif(ENABLE_TP_GLIB_TESTS)
