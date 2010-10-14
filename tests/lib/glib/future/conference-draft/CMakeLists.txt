if(ENABLE_TP_GLIB_TESTS)
    include_directories(
        ${CMAKE_SOURCE_DIR}/tests/lib/glib/future)

    set(future_example_cm_conference_draft_SRCS
        chan.c
        chan.h)

    add_library(future-example-cm-conference-draft STATIC ${future_example_cm_conference_draft_SRCS})
    target_link_libraries(future-example-cm-conference-draft ${TPGLIB_LIBRARIES}
                          tp-glib-tests-future-extensions)
endif(ENABLE_TP_GLIB_TESTS)
