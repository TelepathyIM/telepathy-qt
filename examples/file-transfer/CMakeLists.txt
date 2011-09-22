set(ft-receiver_SRCS
    file-receiver.cpp
    ft-receiver-handler.cpp
    pending-file-receive.cpp)

set(ft-receiver_MOC_SRCS
    file-receiver.h
    ft-receiver-handler.h
    pending-file-receive.h)

tpqt4_generate_mocs(${ft-receiver_MOC_SRCS})

add_executable(ft-receiver ${ft-receiver_SRCS} ${ft-receiver_MOC_SRCS})
target_link_libraries(ft-receiver
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)

set(ft-sender_SRCS
    file-sender.cpp
    file-sender-handler.cpp
    pending-file-send.cpp)

set(ft-sender_MOC_SRCS
    file-sender.h
    file-sender-handler.h
    pending-file-send.h)

tpqt4_generate_mocs(${ft-sender_MOC_SRCS})

add_executable(ft-sender ${ft-sender_SRCS} ${ft-sender_MOC_SRCS})
target_link_libraries(ft-sender
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
