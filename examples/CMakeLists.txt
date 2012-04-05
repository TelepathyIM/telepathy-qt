# Set the required flags found in TelepathyDefaults
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")
set(LD_FLAGS "${LD_FLAGS} ${DEPRECATED_DECLARATIONS_FLAGS}")

add_subdirectory(accounts)
add_subdirectory(contact-messenger)
add_subdirectory(cm)
add_subdirectory(extensions)
add_subdirectory(file-transfer)
add_subdirectory(protocols)
add_subdirectory(roster)
add_subdirectory(stream-tubes)
