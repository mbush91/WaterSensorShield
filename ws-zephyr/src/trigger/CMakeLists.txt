target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/trigger.c
    )
