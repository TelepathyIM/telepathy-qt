if(PINOCCHIO AND PINOCCHIO_CTL)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_tmp)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runPinocchioTest.sh "
    export abs_top_builddir=${CMAKE_BINARY_DIR} 
    export abs_top_srcdir=${CMAKE_SOURCE_DIR}
    export XDG_DATA_HOME=${CMAKE_BINARY_DIR}/tests
    export XDG_DATA_DIRS=${CMAKE_SOURCE_DIR}/tests:$XDG_DATA_DIRS
    export PINOCCHIO=${PINOCCHIO}
    export PINOCCHIO_CTL=${PINOCCHIO_CTL}
    export PINOCCHIO_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    export PINOCCHIO_SAVE_DIR=${CMAKE_CURRENT_BINARY_DIR}/_tmp
    ${SH} ${CMAKE_SOURCE_DIR}/tools/with-session-bus.sh --session -- $@
    ")

    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    qt4_generate_moc_i(lib.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/lib.h.moc.hpp)
    set_source_files_properties(lib.cpp OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_gen/lib.h.moc.hpp)

    function(add_pinochhio_unit_test fancyName name)
        qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
        add_executable(test-pinochhio-${name} ${name}.cpp lib.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
        target_link_libraries(test-pinochhio-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests)
        add_test(${fancyName} ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runPinocchioTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-pinochhio-${name})
    endfunction(add_pinochhio_unit_test fancyName name libraries)

    add_pinochhio_unit_test(PinochhioConnectionManagerBasics cm-basics)
    add_pinochhio_unit_test(PinochhioConnectionBasics conn-basics)
    add_pinochhio_unit_test(PinochhioDoNothing do-nothing)
    add_pinochhio_unit_test(PinochhioHandles handles)
endif(PINOCCHIO AND PINOCCHIO_CTL)
