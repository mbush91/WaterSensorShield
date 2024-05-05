#
# CMakeLists for lib/pss_ble_common/
#
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(app PRIVATE
    pss_nrf_lte.c
    )
