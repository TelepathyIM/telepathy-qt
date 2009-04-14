include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${QT_INCLUDES})

set(roster_SRCS
    main.cpp
    roster-window.cpp
    roster-item.cpp
    roster-widget.cpp)

set(roster_MOC_SRCS
    roster-window.h
    roster-item.h
    roster-widget.h)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
foreach(moc_src ${roster_MOC_SRCS})
    string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    list(APPEND roster_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
endforeach(moc_src ${roster_MOC_SRCS})

add_executable(roster ${roster_SRCS})
target_link_libraries(roster
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)

set(telepathy_qt4_examples_roster_SRCS
    roster-item.cpp
    roster-widget.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-item.moc.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-widget.moc.hpp)

add_library(telepathy-qt4-examples-roster ${telepathy_qt4_examples_roster_SRCS})
target_link_libraries(telepathy-qt4-examples-roster
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
