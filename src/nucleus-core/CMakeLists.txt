cmake_minimum_required(VERSION 3.25)
project(nucleus-core)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(EXPORT_API)

find_package(yaml-cpp REQUIRED)

add_library(nucleus-core SHARED
        api/data.cpp
        api/main.cpp
        api/plugins.cpp
        api/pubsub.cpp
        api/tasks.cpp
        config/config_manager.cpp config/config_manager.h
        config/config_reader.cpp config/config_reader.h
        data/environment.cpp data/environment.h
        data/handle_table.cpp data/handle_table.h
        data/safe_handle.h
        data/shared_struct.cpp data/shared_struct.h
        data/string_table.cpp data/string_table.h
        lifecycle/kernel_command_line.cpp lifecycle/kernel_command_line.h
        plugins/plugin_loader.cpp plugins/plugin_loader.h
        pubsub/local_topics.cpp pubsub/local_topics.h
        tasks/expire_time.h
        tasks/task.cpp tasks/task.h
        )
target_link_libraries(nucleus-core Threads::Threads ${CMAKE_DL_LIBS} ${YAML_CPP_LIBRARIES})
target_link_directories(nucleus-core PUBLIC ${YAML_CPP_LIBRARY_DIR})
target_include_directories(nucleus-core PUBLIC ${YAML_CPP_INCLUDE_DIR})
target_include_directories(nucleus-core PRIVATE .)

add_custom_command(TARGET nucleus-core
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:nucleus-core>" "${TOP_BINARY_DIR}"
        )
