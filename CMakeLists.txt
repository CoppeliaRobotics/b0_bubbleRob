cmake_minimum_required(VERSION 3.15)
project(b0_bubbleRob)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

find_package(Boost REQUIRED COMPONENTS thread system filesystem)

if(NOT LIBPLUGIN_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(LIBPLUGIN_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/libPlugin)
    else()
        set(LIBPLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libPlugin)
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${LIBPLUGIN_DIR}/cmake)
find_package(CoppeliaSim 4.1.0.0 REQUIRED)
find_package(BlueZero REQUIRED)


add_executable(b0_bubbleRob b0_bubbleRob.cpp)
target_compile_definitions(b0_bubbleRob PRIVATE CPP_VERSION)
target_include_directories(b0_bubbleRob PRIVATE ${BLUEZERO_INCLUDE_DIRS})
target_link_libraries(b0_bubbleRob Boost::system)
target_link_libraries(b0_bubbleRob ${BLUEZERO_LIBRARIES})
coppeliasim_add_addon($<TARGET_FILE:b0_bubbleRob>)
