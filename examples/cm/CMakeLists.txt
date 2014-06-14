if(ENABLE_SERVICE_SUPPORT)
    set(cm_SRCS
        protocol.h
        protocol.cpp
        main.cpp)

    set(cm_MOC_SRCS
        protocol.h)

    tpqt_generate_mocs(${cm_MOC_SRCS})

    add_executable(cm ${cm_SRCS} ${cm_MOC_SRCS})
    target_link_libraries(cm
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTNETWORK_LIBRARY}
        ${QT_QTXML_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR}
        telepathy-qt${QT_VERSION_MAJOR}-service
        ${TP_QT_EXECUTABLE_LINKER_FLAGS})
endif(ENABLE_SERVICE_SUPPORT)
