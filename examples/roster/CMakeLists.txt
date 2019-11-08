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

find_package(Qt${QT_VERSION_MAJOR} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets)

add_executable(roster ${roster_SRCS} ${roster_MOC_SRCS})
target_link_libraries(roster
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    TelepathyQt${QT_VERSION_MAJOR}::Core
)

set(telepathy_qt_examples_roster_SRCS
    roster-item.cpp
    roster-widget.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-item.moc.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/_gen/roster-widget.moc.hpp)

add_library(telepathy-qt-examples-roster ${telepathy_qt_examples_roster_SRCS})
target_link_libraries(telepathy-qt-examples-roster
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    TelepathyQt${QT_VERSION_MAJOR}::Core
)
