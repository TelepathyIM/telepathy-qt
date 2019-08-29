if(ENABLE_TP_GLIB_TESTS)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_gen")

    set(tp_glib_tests_future_extensions_SRCS
        extensions.c
        extensions.h)

    set(gen_all_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml)
    add_custom_command(OUTPUT ${gen_all_xml}
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
             ${CMAKE_CURRENT_SOURCE_DIR}/all.xml
             > ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/all.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/channel.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/connection.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/misc.xml
                ${CMAKE_SOURCE_DIR}/tools/xincludator.py)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/gtypes.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/gtypes-body.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/glib-gtypes-generator.py
             ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml
             _gen/gtypes Future
        DEPENDS ${gen_all_xml}
                ${CMAKE_SOURCE_DIR}/tools/glib-gtypes-generator.py)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/enums.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/c-constants-gen.py
             Future ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml
             > _gen/enums.h _gen/enums
        DEPENDS ${gen_all_xml}
                ${CMAKE_SOURCE_DIR}/tools/c-constants-gen.py)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces-body.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces.h
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS ${CMAKE_SOURCE_DIR}/tools/glib-interfaces-gen.py
             Future ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces-body.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces.h
             ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml
        DEPENDS ${gen_all_xml}
                ${CMAKE_SOURCE_DIR}/tools/glib-interfaces-gen.py)

    set(SPECS
        connection
        channel
        misc)
    foreach(spec ${SPECS})
        add_custom_command(OUTPUT "_gen/${spec}.xml"
            COMMAND ${PYTHON_EXECUTABLE}
            ARGS ${CMAKE_SOURCE_DIR}/tools/xincludator.py
                 ${CMAKE_CURRENT_SOURCE_DIR}/${spec}.xml
                 > ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.xml
            DEPENDS ${spec}.xml
                    ${CMAKE_SOURCE_DIR}/tools/xincludator.py)
    endforeach()

    function(SVC_GENERATOR spec)
        set(ARGS
            ${CMAKE_SOURCE_DIR}/tools/glib-ginterface-gen.py
                --filename=${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-${spec}
                --signal-marshal-prefix=_future_ext
                --include='<telepathy-glib/dbus.h>'
                --not-implemented-func='tp_dbus_g_method_return_not_implemented'
                --allow-unstable
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.xml Future_Svc_)
        add_custom_command(OUTPUT _gen/svc-${spec}.c _gen/svc-${spec}.h
            COMMAND ${PYTHON_EXECUTABLE}
            ARGS ${ARGS}
            DEPENDS ${CMAKE_SOURCE_DIR}/tools/glib-ginterface-gen.py
                    ${CMAKE_CURRENT_BINARY_DIR}/_gen/${spec}.xml
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endfunction()

    svc_generator(channel)
    svc_generator(connection)
    svc_generator(misc)

    set(NEW_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/enums.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/gtypes.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/gtypes-body.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/interfaces-body.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-channel.c
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-channel.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection.c
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-connection.h
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-misc.c
        ${CMAKE_CURRENT_BINARY_DIR}/_gen/svc-misc.h)
        list(APPEND tp_glib_tests_future_extensions_SRCS ${NEW_FILES})
        set_source_files_properties(${NEW_FILES} PROPERTIES GENERATED true)

    add_library(tp-glib-tests-future-extensions STATIC ${tp_glib_tests_future_extensions_SRCS})
    target_link_libraries(tp-glib-tests-future-extensions ${TPGLIB_LIBRARIES})
endif()
