#  .----------------.  .----------------.  .----------------.  .-----------------. .----------------. 
# | .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |
# | |     ______   | || |      __      | || |      __      | || | ____  _____  | || |    _______   | |
# | |   .' ___  |  | || |     /  \     | || |     /  \     | || ||_   \|_   _| | || |   /  ___  |  | |
# | |  / .'   \_|  | || |    / /\ \    | || |    / /\ \    | || |  |   \ | |   | || |  |  (__ \_|  | |
# | |  | |         | || |   / ____ \   | || |   / ____ \   | || |  | |\ \| |   | || |   '.___`-.   | |
# | |  \ `.___.'\  | || | _/ /    \ \_ | || | _/ /    \ \_ | || | _| |_\   |_  | || |  |`\____) |  | |
# | |   `._____.'  | || ||____|  |____|| || ||____|  |____|| || ||_____|\____| | || |  |_______.'  | |
# | |              | || |              | || |              | || |              | || |              | |
# | '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |
#  '----------------'  '----------------'  '----------------'  '----------------'  '----------------' 
#
#
# CAUTION! This application is an intellectual propery
#          of LLG Ryszard Gradowski. This application as
#          well as any part of source code cannot be used,
#          modified and distributed by third party person
#          without prior written permission issued by
#          intellectual property owner.
#
# Project anthem:
# Markus Schulz feat. Dauby - Perfect (Agnelli & Nelson remix)
#

cmake_minimum_required(VERSION 3.1.1)
project(caans)

#
# The version number.
#

set(caans_VERSION_MAJOR 1)
set(caans_VERSION_MINOR 0)

#
# Configure a header file to pass some of
# the CMake settings to the source code.
#

configure_file(
  "${PROJECT_SOURCE_DIR}/caans-config.h.in"
  "${PROJECT_SOURCE_DIR}/caans-config.h"
  )

#
# Header .h and .hpp.
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/caans-config.h
     ${PROJECT_SOURCE_DIR}/include/custom_except.hpp
     ${PROJECT_SOURCE_DIR}/include/synchronized_queue.hpp
     ${PROJECT_SOURCE_DIR}/include/thread_worker.hpp
     ${PROJECT_SOURCE_DIR}/include/sms_messenger.hpp
     ${PROJECT_SOURCE_DIR}/include/ipc_server.hpp
     ${PROJECT_SOURCE_DIR}/include/caans_client.hpp
     ${PROJECT_SOURCE_DIR}/include/downloader.hpp
     ${PROJECT_SOURCE_DIR}/include/daemon_process.hpp
)

#
# Sources .c and .cpp.
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/main.cpp
     ${PROJECT_SOURCE_DIR}/downloader.cpp
     ${PROJECT_SOURCE_DIR}/daemon_process.cpp
     ${PROJECT_SOURCE_DIR}/sms_messenger.cpp
     ${PROJECT_SOURCE_DIR}/ipc_server.cpp
     ${PROJECT_SOURCE_DIR}/../3rd_party/easylogging++/easylogging++.cc
     ${PROJECT_SOURCE_DIR}/../3rd_party/cJSON/cJSON.c
)


add_executable(caans ${HEADERS} ${SOURCES})
target_compile_features(caans PRIVATE cxx_range_for)

get_target_property(TEMP caans COMPILE_FLAGS)
if(TEMP STREQUAL "TEMP-NOTFOUND")
    SET(TEMP "") # set to empty string
else()
    SET(TEMP "${TEMP} ")
endif()

SET(TEMP "${TEMP} -DBOOST_SYSTEM_NO_DEPRECATED" )
SET(TEMP "${TEMP} -DBOOST_ERROR_CODE_HEADER_ONLY" )
SET(TEMP "${TEMP} -DBOOST_BIND_GLOBAL_PLACEHOLDERS" )

SET(TEMP "${TEMP} -DELPP_THREAD_SAFE" )
SET(TEMP "${TEMP} -DELPP_STL_LOGGING" )
SET(TEMP "${TEMP} -DELPP_BOOST_LOGGING" )
SET(TEMP "${TEMP} -DELPP_FEATURE_CRASH_LOG" )
SET(TEMP "${TEMP} -DELPP_FEATURE_ALL" )
SET(TEMP "${TEMP} -DELPP_NO_DEFAULT_LOG_FILE" )

set_target_properties(caans PROPERTIES COMPILE_FLAGS ${TEMP} )

#
# Setup nonstandard packages cmake script directory.
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Check for dependences: pthread (Required).
#

find_package (Threads REQUIRED)
target_link_libraries (caans ${CMAKE_THREAD_LIBS_INIT})

#
# Check for dependences: find cURL library (Required).
#

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(caans ${CURL_LIBRARIES})

#
# Static linking boost libraries.
# Linking order is important!
#

set(BOOST_LIBS_PATH "${PROJECT_SOURCE_DIR}/../3rd_party/boost_current_release/stage/lib/")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_atomic.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_chrono.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_date_time.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_system.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_regex.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_thread.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_filesystem.a")
target_link_libraries(caans "${BOOST_LIBS_PATH}/libboost_program_options.a")

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party/easylogging++")

