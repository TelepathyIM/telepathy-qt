set(messenger-sender_SRCS
    sender.cpp)

set(messenger-sender_MOC_SRCS
    sender.h)

tpqt_generate_mocs(${messenger-sender_MOC_SRCS})

add_executable(messenger-sender ${messenger-sender_SRCS} ${messenger-sender_MOC_SRCS})
target_link_libraries(messenger-sender
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTXML_LIBRARY}
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})
