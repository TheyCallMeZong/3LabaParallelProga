cmake_minimum_required(VERSION 3.23)
project(3Laba)

set(CMAKE_CXX_STANDARD 23)

add_executable(3Laba main.cpp delegationmodel.cpp delegationmodel.h)

set(CMAKE_CXX_FLAGS "-fopenmp")
