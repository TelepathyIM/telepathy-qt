set(receiver_SRCS
    receiver.cpp
    receiver-channel.cpp)

set(receiver_MOC_SRCS
    receiver.h
    receiver-channel.h)

tpqt4_generate_mocs(${receiver_MOC_SRCS})

add_executable(receiver ${receiver_SRCS} ${receiver_MOC_SRCS})
target_link_libraries(receiver
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)

set(sender_SRCS
    sender.cpp)

set(sender_MOC_SRCS
    sender.h)

tpqt4_generate_mocs(${sender_MOC_SRCS})

add_executable(sender ${sender_SRCS} ${sender_MOC_SRCS})
target_link_libraries(sender
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
