set(ft-receiver_SRCS
    receiver.cpp
    receiver-channel.cpp)

set(ft-receiver_MOC_SRCS
    receiver.h
    receiver-channel.h)

tpqt4_generate_mocs(${ft-receiver_MOC_SRCS})

add_executable(ft-receiver ${ft-receiver_SRCS} ${ft-receiver_MOC_SRCS})
target_link_libraries(ft-receiver
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)

set(ft-sender_SRCS
    sender.cpp)

set(ft-sender_MOC_SRCS
    sender.h)

tpqt4_generate_mocs(${ft-sender_MOC_SRCS})

add_executable(ft-sender ${ft-sender_SRCS} ${ft-sender_MOC_SRCS})
target_link_libraries(ft-sender
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
