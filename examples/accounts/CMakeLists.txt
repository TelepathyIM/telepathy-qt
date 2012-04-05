set(accounts_SRCS
    main.cpp
    account-item.cpp
    account-item.h
    accounts-window.cpp
    accounts-window.h)

set(accounts_MOC_SRCS
    account-item.h
    accounts-window.h)

tpqt_generate_mocs(${accounts_MOC_SRCS})

add_executable(accounts ${accounts_SRCS} ${accounts_MOC_SRCS})
target_link_libraries(accounts
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTWIDGETS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_EXECUTABLE_LINKER_FLAGS})
