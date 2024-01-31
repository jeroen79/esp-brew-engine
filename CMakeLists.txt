# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

file (STRINGS "version" PROJECT_VER)

project(esp-brew-engine)

configure_file (main/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/main/version.h @ONLY)
