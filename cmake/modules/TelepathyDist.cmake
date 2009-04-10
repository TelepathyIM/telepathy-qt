# setup make dist
add_custom_target(clone git clone ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION})
add_custom_target(clean-git rm -rf ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}/.git*)
add_custom_target(dist tar -czf ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz ${PACKAGE_NAME}-${PACKAGE_VERSION})
add_dependencies(clean-git clone)
add_dependencies(dist clean-git)

# setup make distcheck
FIND_FILE(_distcheck_py distcheck.py PATHS ${CMAKE_MODULE_PATH})
add_custom_target(distcheck ${PYTHON_EXECUTABLE} ${_distcheck_py} ${PACKAGE_NAME} ${PACKAGE_VERSION} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
