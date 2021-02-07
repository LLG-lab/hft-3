#          _____                    _____                _____
#         /\    \                  /\    \              /\    \
#        /::\____\                /::\    \            /::\    \
#       /:::/    /               /::::\    \           \:::\    \
#      /:::/    /               /::::::\    \           \:::\    \
#     /:::/    /               /:::/\:::\    \           \:::\    \
#    /:::/____/               /:::/__\:::\    \           \:::\    \
#   /::::\    \              /::::\   \:::\    \          /::::\    \
#  /::::::\    \   _____    /::::::\   \:::\    \        /::::::\    \
# /:::/\:::\    \ /\    \  /:::/\:::\   \:::\    \      /:::/\:::\    \
#/:::/  \:::\    /::\____\/:::/  \:::\   \:::\____\    /:::/  \:::\____\
#\::/    \:::\  /:::/    /\::/    \:::\   \::/    /   /:::/    \::/    /
# \/____/ \:::\/:::/    /  \/____/ \:::\   \/____/   /:::/    / \/____/
#          \::::::/    /            \:::\    \      /:::/    /
#           \::::/    /              \:::\____\    /:::/    /
#           /:::/    /                \::/    /    \::/    /
#          /:::/    /                  \/____/      \/____/
#         /:::/    /
#        /:::/    /      -=≡≣ High Frequency Trading System  ≣≡=-
#        \::/    /
#         \/____/    Copyright  2017 - 2021 by LLG Ryszard Gradowski
#                                All Rights Reserved.
#
#
# CAUTION! This application is an intellectual propery
#          of LLG Ryszard Gradowski. This application as
#          well as any part of source code cannot be used,
#          modified and distributed by third party person
#          without prior written permission issued by
#          intellectual property owner.
#
# La Card - ... Za Tebe

cmake_minimum_required(VERSION 3.1.1)
project(hft)

#
# The version number.
#

set(hft_VERSION_MAJOR 3)
set(hft_VERSION_MINOR 0)

#
# Configure a header file to pass some of
# the CMake settings to the source code.
#

configure_file(
  "${PROJECT_SOURCE_DIR}/hft-config.h.in"
  "${PROJECT_SOURCE_DIR}/hft-config.h"
  )

#
# Header .h and .hpp.
#

list(APPEND HEADERS
     ${PROJECT_SOURCE_DIR}/hft-config.h
     ${PROJECT_SOURCE_DIR}/include/custom_except.hpp
     ${PROJECT_SOURCE_DIR}/include/auto_deallocator.hpp
     ${PROJECT_SOURCE_DIR}/include/overnight_swaps.hpp
     ${PROJECT_SOURCE_DIR}/include/daemon_process.hpp
     ${PROJECT_SOURCE_DIR}/include/marketplace_gateway_process.hpp
     ${PROJECT_SOURCE_DIR}/include/hft_utils.hpp
     ${PROJECT_SOURCE_DIR}/include/csv_loader.hpp
     ${PROJECT_SOURCE_DIR}/include/mdc.hpp
     ${PROJECT_SOURCE_DIR}/include/granularity_counter.hpp
     ${PROJECT_SOURCE_DIR}/include/range.hpp
     ${PROJECT_SOURCE_DIR}/include/neuron.hpp
     ${PROJECT_SOURCE_DIR}/include/neural_network.hpp
     ${PROJECT_SOURCE_DIR}/include/decision_trigger.hpp
     ${PROJECT_SOURCE_DIR}/include/hftr.hpp
     ${PROJECT_SOURCE_DIR}/include/basic_artifical_inteligence.hpp
     ${PROJECT_SOURCE_DIR}/include/text_file_reader.hpp
     ${PROJECT_SOURCE_DIR}/include/train_stat.hpp
     ${PROJECT_SOURCE_DIR}/include/binomial_approximation.hpp
     ${PROJECT_SOURCE_DIR}/include/fx_account.hpp
     ${PROJECT_SOURCE_DIR}/include/hft_req_proto.hpp
     ${PROJECT_SOURCE_DIR}/include/session_transport.hpp
     ${PROJECT_SOURCE_DIR}/include/basic_tcp_server.hpp
     ${PROJECT_SOURCE_DIR}/include/hft_session.hpp
     ${PROJECT_SOURCE_DIR}/include/position_control.hpp
     ${PROJECT_SOURCE_DIR}/include/position_control_driver.hpp
     ${PROJECT_SOURCE_DIR}/include/position_control_manager.hpp
     ${PROJECT_SOURCE_DIR}/include/trade_time_frame.hpp
     ${PROJECT_SOURCE_DIR}/include/instrument_handler.hpp
     ${PROJECT_SOURCE_DIR}/include/hci.hpp
     ${PROJECT_SOURCE_DIR}/include/files_change_tracker.hpp
     ${PROJECT_SOURCE_DIR}/include/expert_advisor.hpp
     ${PROJECT_SOURCE_DIR}/../caans/include/caans_client.hpp
)

#
# Sources .c and .cpp.
#

list(APPEND SOURCES
     ${PROJECT_SOURCE_DIR}/main.cpp
     ${PROJECT_SOURCE_DIR}/auto_deallocator.cpp
     ${PROJECT_SOURCE_DIR}/hft_utils.cpp
     ${PROJECT_SOURCE_DIR}/overnight_swaps.cpp
     ${PROJECT_SOURCE_DIR}/hftr_generator_main.cpp
     ${PROJECT_SOURCE_DIR}/hftr_mixer_main.cpp
     ${PROJECT_SOURCE_DIR}/hft_bcalc_main.cpp
     ${PROJECT_SOURCE_DIR}/hci_tuner_main.cpp
     ${PROJECT_SOURCE_DIR}/hft_dukascopy_optimizer_main.cpp
     ${PROJECT_SOURCE_DIR}/csv_loader.cpp
     ${PROJECT_SOURCE_DIR}/mdc.cpp
     ${PROJECT_SOURCE_DIR}/neuron.cpp
     ${PROJECT_SOURCE_DIR}/neural_network.cpp
     ${PROJECT_SOURCE_DIR}/decision_trigger.cpp
     ${PROJECT_SOURCE_DIR}/ai_trainer_main.cpp
     ${PROJECT_SOURCE_DIR}/hftr.cpp
     ${PROJECT_SOURCE_DIR}/basic_artifical_inteligence.cpp
     ${PROJECT_SOURCE_DIR}/text_file_reader.cpp
     ${PROJECT_SOURCE_DIR}/train_stat.cpp
     ${PROJECT_SOURCE_DIR}/inst_distribution_main.cpp
     ${PROJECT_SOURCE_DIR}/distrib_approx_main.cpp
     ${PROJECT_SOURCE_DIR}/fx_account.cpp
     ${PROJECT_SOURCE_DIR}/fxemulator_main.cpp
     ${PROJECT_SOURCE_DIR}/binomial_approximation.cpp
     ${PROJECT_SOURCE_DIR}/hft_req_proto.cpp
     ${PROJECT_SOURCE_DIR}/daemon_process.cpp
     ${PROJECT_SOURCE_DIR}/marketplace_gateway_process.cpp
     ${PROJECT_SOURCE_DIR}/trade_time_frame.cpp
     ${PROJECT_SOURCE_DIR}/hft_server_main.cpp
     ${PROJECT_SOURCE_DIR}/session_transport.cpp
     ${PROJECT_SOURCE_DIR}/hft_session.cpp
     ${PROJECT_SOURCE_DIR}/instrument_handler.cpp
     ${PROJECT_SOURCE_DIR}/serial_analyzer_main.cpp
     ${PROJECT_SOURCE_DIR}/position_control_manager.cpp
     ${PROJECT_SOURCE_DIR}/pcp_driver_basic.cpp
     ${PROJECT_SOURCE_DIR}/pcp_driver_chiron.cpp
     ${PROJECT_SOURCE_DIR}/pcp_driver_antychiron.cpp
     ${PROJECT_SOURCE_DIR}/hci.cpp
     ${PROJECT_SOURCE_DIR}/files_change_tracker.cpp
     ${PROJECT_SOURCE_DIR}/expert_advisor.cpp
     ${PROJECT_SOURCE_DIR}/../3rd_party/cJSON/cJSON.c
     ${PROJECT_SOURCE_DIR}/../3rd_party/easylogging++/easylogging++.cc
)


add_executable(hft ${HEADERS} ${SOURCES})
target_compile_features(hft PRIVATE cxx_range_for)

get_target_property(TEMP hft COMPILE_FLAGS)
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

set_target_properties(hft PROPERTIES COMPILE_FLAGS ${TEMP} )


#
# Setup nonstandard packages cmake script directory.
#

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# Check for dependences: pthread (Required).
#

find_package (Threads)
target_link_libraries (hft ${CMAKE_THREAD_LIBS_INIT})

#
# Static linking boost libraries.
# Linking order is important!
#

set(BOOST_LIBS_PATH "${PROJECT_SOURCE_DIR}/../3rd_party/boost_current_release/stage/lib/")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_atomic.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_chrono.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_date_time.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_system.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_regex.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_thread.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_filesystem.a")
target_link_libraries(hft "${BOOST_LIBS_PATH}/libboost_program_options.a")

#
# Additional include directory.
#

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party/boost")
include_directories("${PROJECT_SOURCE_DIR}/../3rd_party/easylogging++")

