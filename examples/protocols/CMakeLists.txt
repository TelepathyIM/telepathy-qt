set(protocols_SRCS
    main.cpp
    cm-wrapper.cpp
    protocols.cpp)

set(protocols_MOC_SRCS
    cm-wrapper.h
    protocols.h)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen" )
foreach(moc_src ${protocols_MOC_SRCS})
    string(REPLACE ".h" ".moc.hpp" generated_file ${moc_src})
    qt4_generate_moc_i(${CMAKE_CURRENT_SOURCE_DIR}/${moc_src} ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
    list(APPEND protocols_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/${generated_file})
endforeach(moc_src ${protocols_MOC_SRCS})

add_executable(protocols ${protocols_SRCS})
target_link_libraries(protocols
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt4)
