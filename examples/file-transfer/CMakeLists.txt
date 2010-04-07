include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${QT_INCLUDES})

set(receiver_SRCS
    receiver.cpp
    receiver-channel.cpp)

set(receiver_MOC_SRCS
    receiver.h
    receiver-channel.h)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
foreach(moc_src ${receiver_MOC_SRCS})
    string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    list(APPEND receiver_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
endforeach(moc_src ${receiver_MOC_SRCS})

add_executable(receiver ${receiver_SRCS})
target_link_libraries(receiver
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)

set(sender_SRCS
    sender.cpp)

set(sender_MOC_SRCS
    sender.h)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
foreach(moc_src ${sender_MOC_SRCS})
    string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    list(APPEND sender_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
endforeach(moc_src ${sender_MOC_SRCS})

add_executable(sender ${sender_SRCS})
target_link_libraries(sender
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
