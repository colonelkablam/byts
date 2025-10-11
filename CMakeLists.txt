cmake_minimum_required(VERSION 3.20)
project(byts LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML (install it and/or set SFML_DIR to its cmake folder)
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics)

# Library with your engine code
add_library(byts_lib
    src/Byt.cpp
    src/World.cpp
)

# Public headers live in include/
target_include_directories(byts_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link SFML to the lib (so consumers inherit it)
target_link_libraries(byts_lib PUBLIC sfml-system sfml-window sfml-graphics)

# Demo executable
add_executable(byts_demo src/main.cpp)
target_link_libraries(byts_demo PRIVATE byts_lib)
