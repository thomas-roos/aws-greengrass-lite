cmake_minimum_required(VERSION 3.25)
project(greengrass_plugin)

set(CMAKE_CXX_STANDARD 17)

add_library(greengrass_plugin SHARED simple_plugin.cpp)
