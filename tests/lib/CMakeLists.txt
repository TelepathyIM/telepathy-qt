include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_INCLUDES}
    ${TPGLIB_INCLUDE_DIRS})

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")
qt4_generate_moc_i(test.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
add_library(tp-qt4-tests test.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
target_link_libraries(tp-qt4-tests ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} telepathy-qt4)

if(ENABLE_TP_GLIB_TESTS)
    set(tp_glib_tests_SRCS
        contacts-conn.c
        contacts-conn.h
        debug.h
        simple-conn.c
        simple-conn.h
        simple-manager.c
        simple-manager.h)
    add_library(tp-glib-tests SHARED ${tp_glib_tests_SRCS})
    target_link_libraries(tp-glib-tests ${TPGLIB_LIBRARIES}
                          tp-glib-csh-tests
                          tp-glib-example-cm-callable
                          tp-glib-example-cm-contactlist
                          tp-glib-example-cm-echo
                          tp-glib-example-cm-echo2)

    add_subdirectory(callable)
    add_subdirectory(contactlist)
    add_subdirectory(csh)
    add_subdirectory(echo)
    add_subdirectory(echo2)
endif(ENABLE_TP_GLIB_TESTS)
