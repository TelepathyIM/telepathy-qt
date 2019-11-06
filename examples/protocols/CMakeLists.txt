set(protocols_SRCS
    main.cpp
    cm-wrapper.cpp
    protocols.cpp)

set(protocols_MOC_SRCS
    cm-wrapper.h
    protocols.h)

tpqt_generate_mocs(${protocols_MOC_SRCS})

add_executable(protocols ${protocols_SRCS} ${protocols_MOC_SRCS})
target_link_libraries(protocols
    Qt5::Core
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})
