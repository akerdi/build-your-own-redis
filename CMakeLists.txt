cmake_minimum_required(VERSION 3.15)
project(learn_redis)

set(CMAKE_CXX_STANDARD 11)
set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 1)

option(HASHTABLE_LIB_MODE "Use to provide HASHTABLE implementation" ON)

configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_SOURCE_DIR}/config.h"
)

if (HASHTABLE_LIB_MODE)
    message("use HashTable")
    include_directories("${PROJECT_SOURCE_DIR}/vendor/hashtable")
    add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/hashtable")
    set(EXTRA_LIB ${EXTRA_LIBS} HASHTABLE)
else()
    message(FATAL_ERROR "cmakedefine must add 'HASHTABLE_LIB_MODE=ON'")
endif()

add_executable(learn_redis main.cpp server.cpp)

target_link_libraries(learn_redis ${EXTRA_LIB})
