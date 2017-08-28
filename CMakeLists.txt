cmake_minimum_required (VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)
project (PomaLoader)

set(BOOST_ROOT "/usr/local")
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package( Boost 1.63.0 REQUIRED COMPONENTS system thread program_options filesystem chrono )
include_directories( ${Boost_INCLUDE_DIRS} )

include(CMakeLists.custom.txt)
include_directories(include "${PROJECT_BINARY_DIR}")
file(GLOB SOURCES_LIB "src/*.cpp")
file(GLOB SOURCES_APP "appsrc/PomaLoader.cpp")
file(GLOB SOURCES_EXAMPLE "appsrc/SampleEmbeddedPipeline.cpp")

if(CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wpedantic")
endif(CMAKE_COMPILER_IS_GNUCC)

add_library(Loader SHARED ${SOURCES_LIB})
target_link_libraries(Loader ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

add_executable(PomaLoader ${SOURCES_APP})
target_link_libraries(PomaLoader Loader)

add_executable(SampleEmbeddedPipeline ${SOURCES_EXAMPLE})
target_link_libraries(SampleEmbeddedPipeline Loader)

