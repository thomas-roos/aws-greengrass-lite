cmake_minimum_required(VERSION 3.25)
project(nucleus-core)

add_compile_definitions(EXPORT_API)

add_library(nucleus-core SHARED
        c_api.cpp
        environment.cpp
        handle_table.cpp
        local_topics.cpp
        main_thread.cpp
        plugin_loader.cpp
        shared_struct.cpp
        string_table.cpp
        task.cpp
        )
target_link_libraries(nucleus-core Threads::Threads ${CMAKE_DL_LIBS})
