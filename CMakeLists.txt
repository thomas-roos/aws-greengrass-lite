cmake_minimum_required(VERSION 3.25)
project(greengrass-lite-all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(UNIX)
    add_compile_definitions(USE_DLFCN NATIVE_SUFFIX=".so")
    add_link_options(-fvisibility=hidden)
elseif(WIN32)
    add_compile_definitions(USE_WINDLL NATIVE_SUFFIX=".dll")
endif()

include_directories(include)

add_subdirectory(src)
