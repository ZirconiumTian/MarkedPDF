cmake_minimum_required(VERSION 3.16)
project(MarkedPDF LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

find_package(PoDoFo REQUIRED)
add_executable(markedpdf ${SRCS})

target_include_directories(markedpdf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(markedpdf PRIVATE podofo_shared)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(markedpdf PRIVATE -Wall -Wextra -Wpedantic)
endif()