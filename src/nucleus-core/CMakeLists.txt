cmake_minimum_required(VERSION 3.25)
project(nucleus-core)

add_compile_definitions(EXPORT_API)

add_library(nucleus-core SHARED
        c_api.cpp
        config.cpp config.h
        environment.cpp environment.h
        expire_time.h
        handle_table.cpp handle_table.h
        local_topics.cpp local_topics.h
        main_thread.cpp
        plugin_loader.cpp plugin_loader.h
        shared_struct.cpp shared_struct.h
        string_table.cpp string_table.h
        task.cpp task.h
        )
target_link_libraries(nucleus-core Threads::Threads ${CMAKE_DL_LIBS})
add_custom_command(TARGET nucleus-core
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nucleus-core>" "${TOP_BINARY_DIR}"
        )
