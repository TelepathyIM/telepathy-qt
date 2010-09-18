if(FARSIGHT_COMPONENTS_FOUND)
    add_definitions(-DQT_NO_KEYWORDS)
    include_directories(
                ${TELEPATHY_FARSIGHT_INCLUDE_DIR}
                ${GSTREAMER_INCLUDE_DIR}
                ${GLIB2_INCLUDE_DIR}
                ${LIBXML2_INCLUDE_DIR}
                ${DBUS_INCLUDE_DIR})

    set(call_SRCS
        main.cpp
        call-handler.cpp
        call-roster-widget.cpp
        call-widget.cpp
        call-window.cpp
        farsight-channel.cpp
        video-widget.cpp)

    set(call_MOC_SRCS
        call-handler.h
        call-roster-widget.h
        call-widget.h
        call-window.h
        farsight-channel.h
        video-widget.h)

    tpqt4_generate_mocs(${call_MOC_SRCS})

    add_executable(call ${call_SRCS} ${call_MOC_SRCS})
    target_link_libraries(call
        ${QT_QTCORE_LIBRARIES}
        ${QT_QTGUI_LIBRARIES}
        ${QT_QTCORE_LIBRARIES}
        telepathy-qt4
        telepathy-qt4-farsight
        telepathy-qt4-examples-roster)
endif(FARSIGHT_COMPONENTS_FOUND)
