cmake_minimum_required(VERSION 3.25)
project(greengrass_lite_experiments)

add_subdirectory(simple-plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(greengrass_lite_experiments main.cpp string_table.cpp c_api.cpp include/c_api.h handle_table.cpp handle_table.h task.cpp task.h environment.cpp environment.h shared_struct.cpp shared_struct.h safe_handle.h include/cpp_api.h local_topics.cpp local_topics.h)
target_link_libraries(greengrass_lite_experiments Threads::Threads)