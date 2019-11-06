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

find_package(Qt5 5.6 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(roster ${roster_SRCS} ${roster_MOC_SRCS})
target_link_libraries(roster
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})

set(telepathy_qt_examples_roster_SRCS
    roster-item.cpp
    roster-widget.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-item.moc.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-widget.moc.hpp)

add_library(telepathy-qt-examples-roster ${telepathy_qt_examples_roster_SRCS})
target_link_libraries(telepathy-qt-examples-roster
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_LIBRARY_LINKER_FLAGS})
