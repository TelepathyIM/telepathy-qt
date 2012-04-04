# This directory is an example of how to build extensions to the spec.
# Typically this would be in a top-level extensions/ directory.

# In this example we build an optional interface for Telepathy Connections,
# so we specify Tp::Client::ConnectionInterface as the main interface for the
# generated proxies with "--mainiface=Tp::Client::ConnectionInterface'. The
# generated proxies will have a convenience constructors for associating the
# proxy with the same remote object an instance of the main interface class
# is associated with. We could instead have made an optional interface for any
# other class, or by leaving that option out entirely we could have made an
# extension that will work on QDBusAbstractInterface or any subclass of it.
#
# For stand-alone interfaces (for which the interface itself should be considered
# the main interface) --mainiface should be specified as fully namespaced name
# of the interface class itself.

set(example_extensions_SRCS
    cli-connection.cpp
    cli-connection.h
    types.cpp)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_gen)
set(generated_all_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/all.xml)
tpqt_xincludator(example-extensions-includator ${CMAKE_CURRENT_SOURCE_DIR}/all.xml ${generated_all_xml})
tpqt_constants_gen(example-extensions-constants ${generated_all_xml} ${CMAKE_CURRENT_BINARY_DIR}/_gen/constants.h
                    --namespace=Example
                    --str-constant-prefix=EXAMPLE_
                    DEPENDS example-extensions-includator)

if(MSVC)
    set(TYPES_INCLUDE ^<TelepathyQt/Types^>)
else(MSVC)
    set(TYPES_INCLUDE '<TelepathyQt/Types>')
endif(MSVC)

tpqt_types_gen(example-extensions-typesgen ${generated_all_xml}
                ${CMAKE_CURRENT_BINARY_DIR}/_gen/types.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/types-body.hpp
                Example types.h types.h
                --extraincludes=${TYPES_INCLUDE}
                DEPENDS example-extensions-constants)

set(connection_generated_xml ${CMAKE_CURRENT_BINARY_DIR}/_gen/connection.xml)
tpqt_xincludator(example-extensions-connection-includator ${CMAKE_CURRENT_SOURCE_DIR}/connection.xml ${connection_generated_xml})

if(MSVC)
  set(ESCAPED_QUOTES \"\"\")
  set(connection_include ^<TelepathyQt/Connection^>)
else(MSVC)
  set(ESCAPED_QUOTES \\\")
  set(connection_include '<TelepathyQt/Connection>')
endif(MSVC)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-body.hpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS ${CMAKE_SOURCE_DIR}/tools/qt-client-gen.py
         --namespace=Example::Client
         --typesnamespace=Example
         --headerfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h
         --implfile=${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-body.hpp
         --realinclude=${CMAKE_CURRENT_SRC_DIR}/cli-connection.h
         --specxml=${generated_all_xml}
         --ifacexml=${connection_generated_xml}
         --extraincludes=${connection_include},${ESCAPED_QUOTES}types.h${ESCAPED_QUOTES}
         --mainiface=Tp::Client::ConnectionInterface)
add_custom_target(example-extensions-connection-generation)
add_dependencies(example-extensions-connection-generation example-extensions-connection-includator example-extensions-typesgen)
list(APPEND example_extensions_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection-body.hpp ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h)
tpqt_generate_moc_i(${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.h ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.moc.hpp)
list(APPEND example_extensions_SRCS ${CMAKE_CURRENT_BINARY_DIR}/_gen/cli-connection.moc.hpp)

add_library(example_extensions STATIC ${example_extensions_SRCS})
add_dependencies(example_extensions example-extensions-connection-generation)
target_link_libraries(example_extensions
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    telepathy-qt${QT_VERSION_MAJOR}
    ${TP_QT_LIBRARY_LINKER_FLAGS})
