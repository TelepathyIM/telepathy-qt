include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${TELEPATHY_GLIB_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${GOBJECT_INCLUDE_DIR}
    ${GIO_INCLUDE_DIR}
    ${GIOUNIX_INCLUDE_DIR}
    ${DBUS_INCLUDE_DIR}
    ${DBUS_ARCH_INCLUDE_DIR})

if(ENABLE_TP_GLIB_TESTS)
    add_subdirectory(call)
    add_subdirectory(callable)
    add_subdirectory(contactlist)
    add_subdirectory(contactlist2)
    add_subdirectory(echo)
    add_subdirectory(echo2)
    add_subdirectory(future)

    set(tp_glib_tests_SRCS
        bug16307-conn.c
        bug16307-conn.h
        captcha-chan.c
        captcha-chan.h
        contacts-conn.c
        contacts-conn.h
        contacts-noroster-conn.c
        contacts-noroster-conn.h
        contact-list-manager.h
        contact-list-manager.c
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
    if(ENABLE_TP_GLIB_GIO_TESTS)
        list(APPEND tp_glib_tests_SRCS dbus-tube-chan.c dbus-tube-chan.h
                                       stream-tube-chan.c stream-tube-chan.h)
    endif(ENABLE_TP_GLIB_GIO_TESTS)
    add_library(tp-glib-tests SHARED ${tp_glib_tests_SRCS})
    target_link_libraries(tp-glib-tests
        ${TELEPATHY_GLIB_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${GOBJECT_LIBRARIES}
        ${GIO_LIBRARIES}
        ${DBUS_GLIB_LIBRARIES}
        ${DBUS_LIBRARIES}
        example-cm-call
        example-cm-callable
        example-cm-contactlist
        example-cm-echo
        example-cm-echo2
        tp-glib-tests-future-extensions)

endif(ENABLE_TP_GLIB_TESTS)

# Add Nemo install target
INSTALL(TARGETS tp-glib-tests LIBRARY DESTINATION ${NEMO_TESTS_DIR_LIB})

