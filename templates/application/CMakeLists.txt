cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(template LANGUAGES C)

add_executable(${PROJECT_NAME} source/template.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

ogc_create_dol(${PROJECT_NAME})

target_compile_options(template PRIVATE -Wall)

