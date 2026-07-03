cmake_minimum_required(VERSION 3.16)
project(sim_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sim_core
    main.cpp
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sim_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
