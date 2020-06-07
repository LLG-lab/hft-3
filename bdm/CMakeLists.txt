#    ____  _ __
#   / __ )(_) /_____ ___  ___  _  __
#  / __  / / __/ __ `__ \/ _ \| |/_/
# / /_/ / / /_/ / / / / /  __/>  <
#/_____/_/\__/_/ /_/ /_/\___/_/|_|
#
#       ____        __
#      / __ \____ _/ /_____ _
#     / / / / __ `/ __/ __ `/
#    / /_/ / /_/ / /_/ /_/ /
#   /_____/\__,_/\__/\__,_/
#
#         __  ___
#        /  |/  /___ _____  ____ _____ ____  _____ (R)
#       / /|_/ / __ `/ __ \/ __ `/ __ `/ _ \/ ___/
#      / /  / / /_/ / / / / /_/ / /_/ /  __/ /
#     /_/  /_/\__,_/_/ /_/\__,_/\__, /\___/_/
#                              /____/
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
# XS Project - Не Стой/Ne Stoi
#

cmake_minimum_required(VERSION 3.1.1)
project(bdm)

#
# The version number.
#

set(bdm_VERSION_MAJOR 1)
set(bdm_VERSION_MINOR 1)

#
# Configure a header file to pass some of
# the CMake settings to the source code.
#

configure_file(
  "${PROJECT_SOURCE_DIR}/bdm-config.h.in"
  "${PROJECT_SOURCE_DIR}/bdm-config.h"
  )

#
# Header .h and .hpp.
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/bdm-config.h
     ${PROJECT_SOURCE_DIR}/include/custom_except.hpp
     ${PROJECT_SOURCE_DIR}/include/file_operations.hpp
     ${PROJECT_SOURCE_DIR}/include/appconfig.hpp
     ${PROJECT_SOURCE_DIR}/include/dialogs.hpp
     ${PROJECT_SOURCE_DIR}/include/bdmdb.hpp
     ${PROJECT_SOURCE_DIR}/include/generator.hpp
     ${PROJECT_SOURCE_DIR}/include/downloader.hpp
)

#
# Sources .c and .cpp.
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/main.cpp
     ${PROJECT_SOURCE_DIR}/bdm_createdb_main.cpp
     ${PROJECT_SOURCE_DIR}/bdm_network_autogen_main.cpp
     ${PROJECT_SOURCE_DIR}/file_operations.cpp
     ${PROJECT_SOURCE_DIR}/dialogs.cpp
     ${PROJECT_SOURCE_DIR}/bdmdb.cpp
     ${PROJECT_SOURCE_DIR}/generator.cpp
     ${PROJECT_SOURCE_DIR}/downloader.cpp
     ${PROJECT_SOURCE_DIR}/../3rd_party/cJSON/cJSON.c
)


add_executable(bdm ${HEADERS} ${SOURCES})
target_compile_features(bdm PRIVATE cxx_range_for)

get_target_property(TEMP bdm COMPILE_FLAGS)
if(TEMP STREQUAL "TEMP-NOTFOUND")
    SET(TEMP "") # set to empty string
else()
    SET(TEMP "${TEMP} ")
endif()

SET(TEMP "${TEMP} -DBOOST_SYSTEM_NO_DEPRECATED" )
SET(TEMP "${TEMP} -DBOOST_ERROR_CODE_HEADER_ONLY" )

SET(TEMP "${TEMP} -DELPP_THREAD_SAFE" )
SET(TEMP "${TEMP} -DELPP_STL_LOGGING" )
SET(TEMP "${TEMP} -DELPP_BOOST_LOGGING" )
SET(TEMP "${TEMP} -DELPP_FEATURE_CRASH_LOG" )
SET(TEMP "${TEMP} -DELPP_FEATURE_ALL" )
SET(TEMP "${TEMP} -DELPP_NO_DEFAULT_LOG_FILE" )

set_target_properties(bdm PROPERTIES COMPILE_FLAGS ${TEMP} )


#
# Setup nonstandard packages cmake script directory.
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Check for dependences: pthread (Required).
#

find_package (Threads REQUIRED)
target_link_libraries (bdm ${CMAKE_THREAD_LIBS_INIT})

#
# Check for dependences: find cURL library (Required).
#

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(bdm ${CURL_LIBRARIES})

#
# Check for dependences: SQLite3 (Required).
#

find_package (SQLite3 REQUIRED)
if (NOT ${SQLITE3_FOUND})
    message(FATAL_ERROR "Package SQLite3 required, but not found!")
endif (NOT ${SQLITE3_FOUND})

include_directories(${SQLITE3_INCLUDE_DIRS})
target_link_libraries (bdm ${SQLITE3_LIBRARIES})

#
# Check for dependences: newt (Required).
#

find_package(newt REQUIRED)

if (NOT ${NEWT_FOUND})
    message(FATAL_ERROR "Package newt required, but not found!")
endif (NOT ${NEWT_FOUND})

include_directories(${NEWT_INCLUDE_DIRS})
target_link_libraries (bdm ${NEWT_LIBRARIES})

#
# Static linking boost libraries.
# Linking order is important!
#

set(BOOST_LIBS_PATH "${PROJECT_SOURCE_DIR}/../3rd_party/boost_current_release/stage/lib/")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_atomic.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_chrono.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_date_time.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_system.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_regex.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_thread.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_filesystem.a")
target_link_libraries(bdm "${BOOST_LIBS_PATH}/libboost_program_options.a")

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party/boost")

