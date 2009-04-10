include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${QT_INCLUDES})

find_program(SH sh)

set(test_environment "
export abs_top_builddir=${CMAKE_BINARY_DIR}
export abs_top_srcdir=${CMAKE_SOURCE_DIR}
export XDG_DATA_HOME=${CMAKE_BINARY_DIR}/tests
export XDG_DATA_DIRS=${CMAKE_SOURCE_DIR}/tests
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh "${test_environment} $@")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
function(add_generic_unit_test fancyName name)
    qt4_generate_moc_i(${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    add_executable(test-${name} ${name}.cpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/${name}.cpp.moc.hpp)
    target_link_libraries(test-${name} ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} telepathy-qt4 tp-qt4-tests)
    add_test(${fancyName} ${SH} ${CMAKE_CURRENT_BINARY_DIR}/runGenericTest.sh ${CMAKE_CURRENT_BINARY_DIR}/test-${name})
endfunction(add_generic_unit_test fancyName name)

add_generic_unit_test(KeyFile key-file)
add_generic_unit_test(ManagerFile manager-file)

# is dbus-python installed?
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import dbus.mainloop.glib"
            RESULT_VARIABLE PYTHON_DBUS_RESULT)
if(PYTHON_DBUS_RESULT EQUAL 0)
    message(STATUS "dbus-python installed, running additional tests")
    set(HAVE_TEST_PYTHON 1)
else(PYTHON_DBUS_RESULT EQUAL 0)
    message( STATUS "dbus-python not installed, skipping some tests" )
    set(HAVE_TEST_PYTHON 0)
endif(PYTHON_DBUS_RESULT EQUAL 0)

# is Qt4 compiled with Glib support?
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
set(CMAKE_REQUIRED_DEFINITIONS "")
set(CMAKE_REQUIRED_FLAGS "")

CHECK_CXX_SOURCE_COMPILES("
#include <QtCore/QtGlobal>
int main()
{
#if defined(QT_NO_GLIB)
#error \"Qt was compiled with Glib disabled\"
#endif
return 0;
}"
QT4_GLIB_SUPPORT)

# find telepathy-glib
find_package(PkgConfig REQUIRED)
pkg_check_modules(TPGLIB telepathy-glib)

# process results...
if(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)
    message(STATUS "Qt4 compiled with Glib support and telepathy-glib found, running additional tests.")
    set(ENABLE_TP_GLIB_TESTS 1)
else(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)
    message( STATUS "Telepathy-glib not found or Qt4 not compiled with glib support, disabling some tests.")
    set(ENABLE_TP_GLIB_TESTS 0)
endif(QT4_GLIB_SUPPORT AND TPGLIB_FOUND)

add_subdirectory(dbus-1/services)
add_subdirectory(lib)
add_subdirectory(dbus)
add_subdirectory(pinocchio)
