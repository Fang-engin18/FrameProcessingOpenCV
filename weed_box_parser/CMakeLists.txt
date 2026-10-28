
cmake_minimum_required(VERSION 3.20)
project(weed_box_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR
    "C:/Users/fang/Desktop/study/libraryCPP/opencv/build"
)

find_package(OpenCV REQUIRED CONFIG)

add_executable(weed_box_parser
    main.cpp
    sectors.cpp
)

target_include_directories(weed_box_parser PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(weed_box_parser PRIVATE
    ${OpenCV_LIBS}
)
