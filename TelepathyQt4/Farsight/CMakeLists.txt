if(FARSIGHT_COMPONENTS_FOUND)
    include_directories(${TELEPATHY_FARSIGHT_INCLUDE_DIR}
                        ${GSTREAMER_INCLUDE_DIR}
                        ${GLIB2_INCLUDE_DIR}
                        ${LIBXML2_INCLUDE_DIR}
                        ${DBUS_INCLUDE_DIR})

    set(telepathy_qt4_farsight_SRCS
        channel.cpp)

    set(telepathy_qt4_farsight_HEADERS
        Channel
        channel.h)

    add_library(telepathy-qt4-farsight SHARED ${telepathy_qt4_farsight_SRCS})
    target_link_libraries(telepathy-qt4-farsight
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTCORE_LIBRARY}
        ${TELEPATHY_FARSIGHT_LIBRARIES}
        ${GSTREAMER_INTERFACE_LIBRARY}
        telepathy-qt4)

    install(TARGETS telepathy-qt4-farsight DESTINATION ${LIB_INSTALL_DIR})
    install(FILES ${telepathy_qt4_farsight_HEADERS} DESTINATION ${INCLUDE_INSTALL_DIR}/telepathy-1.0/TelepathyQt4/Farsight)
endif(FARSIGHT_COMPONENTS_FOUND)
