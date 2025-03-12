cmake_minimum_required(VERSION 3.24)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(CMAKE_PROJECT_NAME DCP_Validator)

project(${CMAKE_PROJECT_NAME} LANGUAGES C ASM)

