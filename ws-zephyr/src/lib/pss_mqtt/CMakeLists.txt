# CMakeLists for pss_mqtt
# Header files are target_sources so CMake can find them
set(CERT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cert)

if(EXISTS ${CERT_DIR})
  execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/CredentialsGen.py RESULT_VARIABLE ret)
  if(NOT ret EQUAL 0)
    message(FATAL_ERROR "cert generator didn't work")
  endif()
else()
  # The folder does not exist
  message(STATUS "No Provisioning. The folder ${CERT_DIR} does not exist.")
endif()


target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/pss_mqtt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pss_mqtt.h
    )

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
