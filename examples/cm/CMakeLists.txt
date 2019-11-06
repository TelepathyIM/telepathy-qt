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
        Qt5::Core
        telepathy-qt${QT_VERSION_MAJOR}
        telepathy-qt${QT_VERSION_MAJOR}-service
        ${TP_QT_EXECUTABLE_LINKER_FLAGS})
endif()
