cmake_minimum_required(VERSION 3.25)
project(simple_plugin1)

add_library(simple_plugin1 SHARED simple_plugin1.cpp)
target_link_libraries(simple_plugin1 nucleus-core)
