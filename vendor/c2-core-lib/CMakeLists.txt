cmake_minimum_required(VERSION 3.20)

project(C2Core)

set(CORE_SOURCES
    src/time_core.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)