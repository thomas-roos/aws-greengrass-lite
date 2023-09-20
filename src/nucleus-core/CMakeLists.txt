cmake_minimum_required(VERSION 3.25)
project(nucleus-core)

add_compile_definitions(EXPORT_API)

add_library(nucleus-core SHARED
        string_table.cpp
        c_api.cpp
        handle_table.cpp
        task.cpp
        environment.cpp
        shared_struct.cpp
        local_topics.cpp
        plugin_loader.cpp
        )
target_link_libraries(nucleus-core Threads::Threads ${CMAKE_DL_LIBS})
