set(roster_SRCS
    main.cpp
    roster-window.cpp
    roster-item.cpp
    roster-widget.cpp)

set(roster_MOC_SRCS
    roster-window.h
    roster-item.h
    roster-widget.h)

tpqt_generate_mocs(${roster_MOC_SRCS})

add_executable(roster ${roster_SRCS} ${roster_MOC_SRCS})
target_link_libraries(roster
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTWIDGETS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

set(telepathy_qt_examples_roster_SRCS
    roster-item.cpp
    roster-widget.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-item.moc.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-widget.moc.hpp)

add_library(telepathy-qt-examples-roster ${telepathy_qt_examples_roster_SRCS})
target_link_libraries(telepathy-qt-examples-roster
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTWIDGETS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_LIBRARY_LINKER_FLAGS})
