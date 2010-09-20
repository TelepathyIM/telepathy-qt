include_directories(
    ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")
tpqt4_generate_moc_i(test.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
add_library(tp-qt4-tests test.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/test.h.moc.hpp)
target_link_libraries(tp-qt4-tests ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} telepathy-qt4)

if(ENABLE_TP_GLIB_TESTS)
    add_subdirectory(glib)
endif(ENABLE_TP_GLIB_TESTS)
