# Some useful commands
add_custom_command(OUTPUT FIXME.out

                   COMMAND egrep

                   ARGS -A 5 '[F]IXME|[T]ODO|[X]XX'  ${CMAKE_SOURCE_DIR}/TelepathyQt4/*.[ch]*
                         ${CMAKE_SOURCE_DIR}/TelepathyQt4/*.[ch]*
                        > FIXME.out || true)
add_custom_target(check-local DEPENDS FIXME.out)

execute_process(COMMAND ${SH} tools/git-which-branch.sh misc | tr -d '\n' | tr -C "[:alnum:]" _
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_BRANCH_CURRENT)

if (GIT_BRANCH_CURRENT)
    string(LENGTH ${GIT_BRANCH_CURRENT} HAVE_GIT_BRANCH)

    if (HAVE_GIT_BRANCH)
        string(REPLACE "\n" "" GIT_BRANCH_CURRENT ${GIT_BRANCH_CURRENT})

        set(UPLOAD_BRANCH_TO people.freedesktop.org:public_html/telepathy-qt4)

        add_custom_target(upload-branch-docs rsync -rtzvPp --chmod=a+rX doc/html/ ${UPLOAD_BRANCH_TO}-${GIT_BRANCH_CURRENT}
                                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        add_dependencies(upload-branch-docs doxygen-doc)
    endif (HAVE_GIT_BRANCH)
endif (GIT_BRANCH_CURRENT)

add_custom_target(maintainer-upload-release-docs
        rsync -rtOvzPp --chmod=Dg+s,ug+rwX,o=rX doc/html/ telepathy.freedesktop.org:/srv/telepathy.freedesktop.org/www/doc/telepathy-qt4/

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(maintainer-upload-release-docs doxygen-doc _maintainer-upload-release-check)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/maintainer-upload-release-check.sh "
#!/bin/sh
case ${PACKAGE_VERSION} in
        (*.*.*.*)
                echo \"${PACKAGE_VERSION} is not a release\" >&2;
                exit 2;
                ;;
esac
test -f ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
if ! test -f ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz.asc; then
    gpg --detach-sign -a ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz;
fi;
gpg --verify ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz.asc
")

add_custom_target(_maintainer-upload-release-check ${SH} ${CMAKE_CURRENT_BINARY_DIR}/maintainer-upload-release-check.sh)

add_custom_target(maintainer-upload-release
        rsync -vzP ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
telepathy.freedesktop.org:/srv/telepathy.freedesktop.org/www/releases/${PACKAGE_NAME}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz
        COMMAND
        rsync -vzP ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz.asc
telepathy.freedesktop.org:/srv/telepathy.freedesktop.org/www/releases/${PACKAGE_NAME}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz.asc)
add_dependencies(maintainer-upload-release _maintainer-upload-release-check maintainer-upload-release-docs)


set(toolchain_files
    c-constants-gen.py
    check-misc.sh
    check-whitespace.sh
    git-which-branch.sh
    glib-ginterface-gen.py
    glib-gtypes-generator.py
    glib-interfaces-gen.py
    glib-signals-marshal-gen.py
    libtpcodegen.py
    libglibcodegen.py
    libqt4codegen.py
    qt4-client-gen.py
    qt4-constants-gen.py
    qt4-types-gen.py
    manager-file.py
    with-session-bus.sh
    xincludator.py
)

string(REPLACE "." " " sh_toolchain_files ${toolchain_files})

set(TELEPATHY_SPEC_SRCDIR ${CMAKE_SOURCE_DIR}/../telepathy-spec)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/maintainer-update-from-telepathy-spec.sh "
#!/bin/sh
set -e
cd ${CMAKE_SOURCE_DIR}
for x in ${sh_toolchain_files}; do
    if test -f ${TELEPATHY_SPEC_SRCDIR}/tools/$$x; then
        cp ${TELEPATHY_SPEC_SRCDIR}/tools/$$x $$x;
    fi;
done
")
add_custom_target(maintainer-update-from-telepathy-spec ${SH} ${CMAKE_CURRENT_BINARY_DIR}/maintainer-update-from-telepathy-spec.sh
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

set(TELEPATHY_GLIB_SRCDIR ${CMAKE_SOURCE_DIR}/../telepathy-glib)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/maintainer-update-from-telepathy-glib.sh "
#!/bin/sh
set -e
cd ${CMAKE_SOURCE_DIR}
for x in ${sh_toolchain_files}; do
    if test -f ${TELEPATHY_GLIB_SRCDIR}/tools/$$x; then
        cp ${TELEPATHY_GLIB_SRCDIR}/tools/$$x $$x;
    fi;
done
")
add_custom_target(maintainer-update-from-telepathy-glib ${SH} ${CMAKE_CURRENT_BINARY_DIR}/maintainer-update-from-telepathy-glib.sh
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#lcov
add_custom_target(lcov-reset lcov --directory ${CMAKE_BINARY_DIR} --zerocounters
                             COMMAND find ${CMAKE_BINARY_DIR} -name '*.gcda' -exec rm -f '{}' ';' || true
                             COMMENT "Cleaning lcov files")

add_custom_target(lcov-check make test || true
                             COMMAND lcov --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/lcov.info &&
                                     mkdir ${CMAKE_BINARY_DIR}/lcov.html || true && genhtml --title ${PACKAGE_NAME}
                                     --output-directory ${CMAKE_BINARY_DIR}/lcov.html ${CMAKE_BINARY_DIR}/lcov.info
                             COMMENT "Generating lcov report in file://${CMAKE_BINARY_DIR}/lcov.html/index.html"
                             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
add_dependencies(lcov-check lcov-reset)
