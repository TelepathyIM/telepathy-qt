set(tubereceiver_SRCS
    tube-receiver.cpp)

set(tubereceiver_MOC_SRCS
    tube-receiver.h)

tpqt_generate_mocs(${tubereceiver_MOC_SRCS})

add_executable(tubereceiver ${tubereceiver_SRCS} ${tubereceiver_MOC_SRCS})
target_link_libraries(tubereceiver
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    TelepathyQt${QT_VERSION_MAJOR}::Core
)

set(tubeinitiator_SRCS
    tube-initiator.cpp)

set(tubeinitiator_MOC_SRCS
    tube-initiator.h)

tpqt_generate_mocs(${tubeinitiator_MOC_SRCS})

add_executable(tubeinitiator ${tubeinitiator_SRCS} ${tubeinitiator_MOC_SRCS})
target_link_libraries(tubeinitiator
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    TelepathyQt${QT_VERSION_MAJOR}::Core
)
