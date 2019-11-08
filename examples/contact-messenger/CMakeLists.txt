set(messenger-sender_SRCS
    sender.cpp)

set(messenger-sender_MOC_SRCS
    sender.h)

tpqt_generate_mocs(${messenger-sender_MOC_SRCS})

add_executable(messenger-sender ${messenger-sender_SRCS} ${messenger-sender_MOC_SRCS})
target_link_libraries(messenger-sender
    Qt${QT_VERSION_MAJOR}::Core
    TelepathyQt${QT_VERSION_MAJOR}::Core
)
