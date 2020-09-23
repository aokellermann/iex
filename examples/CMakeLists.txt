cmake_minimum_required(VERSION 3.16)

project(example
        VERSION 0.0.1
        LANGUAGES CXX
        DESCRIPTION "Example description"
        )

set(CMAKE_CXX_STANDARD 17)

find_package(iex REQUIRED)

add_executable(example example.cc)
target_link_libraries(example iex::iex)
