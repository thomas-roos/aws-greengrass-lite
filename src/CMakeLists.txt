cmake_minimum_required(VERSION 3.25)
project(greengrass-lite)

add_subdirectory(nucleus-core)
add_subdirectory(simple-plugin)

add_executable(greengrass-lite main.cpp)
target_link_libraries(greengrass-lite Threads::Threads ${CMAKE_DL_LIBS} nucleus-core)
