set(accounts_SRCS
    main.cpp
    account-item.cpp
    account-item.h
    accounts-window.cpp
    accounts-window.h)

set(accounts_MOC_SRCS
    account-item.h
    accounts-window.h)

tpqt4_generate_mocs(${accounts_MOC_SRCS})

add_executable(accounts ${accounts_SRCS} ${accounts_MOC_SRCS})
target_link_libraries(accounts
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
