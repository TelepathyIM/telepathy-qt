include_directories(
    ${CMAKE_CURRENT_BINARY_DIR})

set(tp_qt_tests_SRCS
    test.cpp
    test-thread-helper.cpp
)

set(tp_qt_tests_MOC_SRCS
    test.h
    test-thread-helper.h
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

foreach(moc_src ${tp_qt_tests_MOC_SRCS})
    set(generated_file _gen/${moc_src})
    string(REPLACE ".h" ".h.moc.hpp" generated_file ${generated_file})
    tpqt_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src}
                        ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
    list(APPEND tp_qt_tests_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${generated_file})
endforeach(moc_src ${tp_qt_tests_MOC_SRCS})

add_library(tp-qt-tests ${tp_qt_tests_SRCS})
target_link_libraries(tp-qt-tests ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTTEST_LIBRARY} telepathy-qt${QT_VERSION_MAJOR})

if(ENABLE_TP_GLIB_TESTS)
    add_subdirectory(glib)
    add_subdirectory(glib-helpers)
endif(ENABLE_TP_GLIB_TESTS)

# Add Nemo install target
INSTALL(TARGETS tp-qt-tests LIBRARY DESTINATION ${NEMO_TESTS_DIR_LIB})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/python/account-manager.py DESTINATION ${NEMO_TESTS_DIR_BIN} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
