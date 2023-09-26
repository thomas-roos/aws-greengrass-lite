cmake_minimum_required(VERSION 3.25)
project(nucleus-core)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(EXPORT_API)

find_package(yaml-cpp REQUIRED)

add_library(nucleus-core SHARED
        c_api.cpp
        config_manager.cpp config_manager.h
        config_reader.cpp config_reader.h
        environment.cpp environment.h
        expire_time.h
        handle_table.cpp handle_table.h
        kernel_command_line.cpp kernel_command_line.h
        local_topics.cpp local_topics.h
        plugin_loader.cpp plugin_loader.h
        shared_struct.cpp shared_struct.h
        string_table.cpp string_table.h
        task.cpp task.h
        )
target_link_libraries(nucleus-core Threads::Threads ${CMAKE_DL_LIBS} ${YAML_CPP_LIBRARIES})
target_link_directories(nucleus-core PUBLIC ${YAML_CPP_LIBRARY_DIR})
target_include_directories(nucleus-core PUBLIC ${YAML_CPP_INCLUDE_DIR})

add_custom_command(TARGET nucleus-core
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nucleus-core>" "${TOP_BINARY_DIR}"
        )
