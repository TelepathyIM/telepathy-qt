# setup make dist
add_custom_target(dist cd ${CMAKE_SOURCE_DIR} &&
                        git archive --format=tar --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD |
                            gzip > ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz)

# setup make distcheck
add_custom_target(distcheck cd ${CMAKE_BINARY_DIR} &&
                        rm -rf ${PACKAGE_NAME}-${PACKAGE_VERSION} &&
                        gzip -d ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz &&
                        tar -xf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar &&
                        cd ${PACKAGE_NAME}-${PACKAGE_VERSION}/ &&
                        cmake . && make && make test && make doxygen-doc &&
                        cd ${CMAKE_BINARY_DIR} &&
                        tar -rf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar ${PACKAGE_NAME}-${PACKAGE_VERSION}/doc/ &&
                        gzip ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar)
add_dependencies(distcheck dist)
