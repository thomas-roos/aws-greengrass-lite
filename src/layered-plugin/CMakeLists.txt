cmake_minimum_required(VERSION 3.25)
project(layered_plugin)

add_library(layered_plugin SHARED layered_plugin.cpp)
target_link_libraries(layered_plugin nucleus-core)
