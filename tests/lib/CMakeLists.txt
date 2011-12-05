include_directories(
    ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")
tpqt_generate_moc_i(test.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
add_library(tp-qt-tests test.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
target_link_libraries(tp-qt-tests ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} telepathy-qt${QT_VERSION_MAJOR})

if(ENABLE_TP_GLIB_TESTS)
    add_subdirectory(glib)
    add_subdirectory(glib-helpers)
endif(ENABLE_TP_GLIB_TESTS)
