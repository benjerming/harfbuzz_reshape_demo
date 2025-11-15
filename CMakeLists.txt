cmake_minimum_required(VERSION 3.10.0)
project(demo VERSION 0.1.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找HarfBuzz和FreeType库
find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(FREETYPE REQUIRED freetype2)

add_executable(demo demo.cpp)

# 链接库
target_include_directories(demo PRIVATE 
    ${HARFBUZZ_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(demo 
    ${HARFBUZZ_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

target_link_directories(demo PRIVATE
    ${HARFBUZZ_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)
