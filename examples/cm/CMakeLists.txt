if(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
    set(cm_SRCS
        main.cpp)

    add_executable(cm ${cm_SRCS})
    target_link_libraries(cm
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${QT_QTNETWORK_LIBRARY}
        ${QT_QTXML_LIBRARY}
        telepathy-qt${QT_VERSION_MAJOR}
        telepathy-qt${QT_VERSION_MAJOR}-service)
endif(ENABLE_EXPERIMENTAL_SERVICE_SUPPORT)
