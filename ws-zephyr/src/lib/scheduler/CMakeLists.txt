# CMakeLists for scheduler
# Header files are target_sources so CMake can find them

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/scheduler_runnable_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/scheduler_cfg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/scheduler_task_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/scheduler_timer_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scheduler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scheduler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scheduler_private.h
    )

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
