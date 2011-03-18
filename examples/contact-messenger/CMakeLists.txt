set(messenger-sender_SRCS
    sender.cpp)

set(messenger-sender_MOC_SRCS
    sender.h)

tpqt4_generate_mocs(${messenger-sender_MOC_SRCS})

add_executable(messenger-sender ${messenger-sender_SRCS} ${messenger-sender_MOC_SRCS})
target_link_libraries(messenger-sender
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
