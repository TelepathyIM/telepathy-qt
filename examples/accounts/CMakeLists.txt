include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${QT_INCLUDES})

set(accounts_SRCS
    main.cpp
    account-item.cpp
    account-item.h
    accounts-window.cpp
    accounts-window.h)

set(accounts_MOC_SRCS
    account-item.h
    accounts-window.h)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")
foreach(moc_src ${accounts_MOC_SRCS})
    string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    list(APPEND accounts_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
endforeach(moc_src ${accounts_SRCS})

add_executable(accounts ${accounts_SRCS})
target_link_libraries(accounts
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
