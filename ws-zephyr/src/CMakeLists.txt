#
# CMake file for nrf52840_pss_ble src
#

add_subdirectory(lib)
add_subdirectory(trigger)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    )
