if(TP_FARSIGHT_FOUND)
    include_directories(${CMAKE_SOURCE_DIR}
                        ${CMAKE_BINARY_DIR}
                        ${QT_INCLUDES}
                        ${TP_FARSIGHT_INCLUDE_DIRS})

    set(telepathy_qt4_farsight_SRCS
        channel.cpp)

    set(telepathy_qt4_farsight_HEADERS
        Channel
        channel.h)

    add_library(telepathy-qt4-farsight STATIC ${telepathy_qt4_farsight_SRCS})
    target_link_libraries(telepathy-qt4-farsight
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${TP_FARSIGHT_LIBRARIES}
        telepathy-qt4)

    install(TARGETS telepathy-qt4-farsight DESTINATION ${LIB_INSTALL_DIR})
    install(FILES ${telepathy_qt4_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4/Farsight)

    # pkg-config file
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TelepathyQt4Farsight.pc.in ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4Farsight.pc)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TelepathyQt4Farsight.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
endif(TP_FARSIGHT_FOUND)
