include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${TELEPATHY_GLIB_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIR}
    ${GOBJECT_INCLUDE_DIR}
    ${DBUS_INCLUDE_DIR})

add_definitions(-DQT_NO_KEYWORDS)

if(ENABLE_TP_GLIB_TESTS)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")
    tpqt_generate_moc_i(test-conn-helper.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/test-conn-helper.moc.hpp)
    add_library(tp-qt-tests-glib-helpers test-conn-helper.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/test-conn-helper.moc.hpp)
    target_link_libraries(tp-qt-tests-glib-helpers
        ${TELEPATHY_GLIB_LIBRARIES}
        ${GOBJECT_LIBRARIES}
        ${GLIB2_LIBRARIES}
        ${DBUS_GLIB_LIBRARIES}
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTTEST_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR})
endif(ENABLE_TP_GLIB_TESTS)

# Add Nemo install target
INSTALL(TARGETS tp-qt-tests-glib-helpers LIBRARY DESTINATION ${NEMO_TESTS_DIR_LIB})
