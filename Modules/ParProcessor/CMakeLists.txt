cmake_minimum_required (VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)
get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId} C CXX)
include( ${CMAKE_CURRENT_LIST_DIR}/../../CMakeLists.custom.txt )
# Boost dependency

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package( Boost 1.63.0 REQUIRED COMPONENTS system thread program_options filesystem chrono )
include_directories( ${Boost_INCLUDE_DIRS} )

include_directories(include ../../include)
file(GLOB SOURCES "src/*.cpp")
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${CUSTOM_TARGET_LINK_LIBS})
