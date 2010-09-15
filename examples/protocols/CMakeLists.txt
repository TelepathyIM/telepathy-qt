set(protocols_SRCS
    main.cpp
    cm-wrapper.cpp
    protocols.cpp)

set(protocols_MOC_SRCS
    cm-wrapper.h
    protocols.h)

tpqt4_generate_mocs(${protocols_MOC_SRCS})

add_executable(protocols ${protocols_SRCS} ${protocols_MOC_SRCS})
target_link_libraries(protocols
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
