cmake_minimum_required(VERSION 3.16)
project(byts LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

add_executable(byts 
    src/main.cpp
    src/Byt.cpp
    src/World.cpp
)
target_link_libraries(byts PRIVATE sfml-system sfml-window sfml-graphics)
