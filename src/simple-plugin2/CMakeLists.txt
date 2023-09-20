cmake_minimum_required(VERSION 3.25)
project(simple_plugin2)

add_library(simple_plugin2 SHARED simple_plugin2.cpp)
target_link_libraries(simple_plugin2 nucleus-core)
