include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${TELEPATHY_GLIB_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${GOBJECT_INCLUDE_DIR}
    ${DBUS_INCLUDE_DIR})

if(ENABLE_TP_GLIB_TESTS)
    add_subdirectory(callable)
    add_subdirectory(contactlist)
    add_subdirectory(csh)
    add_subdirectory(echo)
    add_subdirectory(echo2)
    add_subdirectory(future)

    set(tp_glib_tests_SRCS
        contacts-conn.c
        contacts-conn.h
        contact-search-chan.c
        contact-search-chan.h
        debug.h
        params-cm.c
        params-cm.h
        simple-account.c
        simple-account.h
        simple-account-manager.c
        simple-account-manager.h
        simple-channel-dispatch-operation.c
        simple-channel-dispatch-operation.h
        simple-client.c
        simple-client.h
        simple-conn.c
        simple-conn.h
        simple-manager.c
        simple-manager.h
        textchan-group.c
        textchan-group.h
        textchan-null.c
        textchan-null.h
        util.c
        util.h)
    add_library(tp-glib-tests SHARED ${tp_glib_tests_SRCS})
    target_link_libraries(tp-glib-tests ${TELEPATHY_GLIB_LIBRARIES}
                          example-cm-callable
                          example-cm-contactlist
                          example-cm-csh
                          example-cm-echo
                          example-cm-echo2
                          tp-glib-tests-future-extensions
                          future-example-cm-call
                          future-example-cm-conference
                          future-example-cm-conference-draft)

endif(ENABLE_TP_GLIB_TESTS)